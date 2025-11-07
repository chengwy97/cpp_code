#pragma once

#include <atomic>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/system/detail/error_code.hpp>
#include <chrono>
#include <future>
#include <mutex>
#include <optional>
#include <string>
#include <tl/tl_expected.hpp>

#include "thread_pool.hpp"

namespace task {

// 用户下发的控制命令
enum class TaskCommand {
    CANCEL,  // 取消任务
    PAUSE,   // 暂停任务
    RESUME,  // 恢复任务
};

// 任务反馈的实际状态
enum class TaskStatus {
    INITIAL,    // 任务初始状态
    RUNNING,    // 任务运行中
    COMPLETED,  // 任务完成
    FAILED,     // 任务失败
    CANCELLED,  // 任务已取消
    PAUSED,     // 任务已暂停
};

template <typename T>
class Task : public std::enable_shared_from_this<Task<T>> {
   public:
    using InputParamemetersChannelSignature = void(boost::system::error_code, T);
    using TaskControlChannelSignature       = void(boost::system::error_code, TaskCommand);

    enum class TaskResult {
        SUCCESS,
        FAILURE,
        CANCELLED,
    };

    Task(const std::string& name)
        : name_(name),
          input_parameters_channel_(
              ThreadPool::instance().get_coroutine_io_context().get_executor(), 1),
          task_control_channel_(ThreadPool::instance().get_coroutine_io_context().get_executor(),
                                1) {}
    virtual ~Task() {
        input_parameters_channel_.close();
        task_control_channel_.close();
    }

    void run() {
        std::lock_guard<std::mutex> lock(current_status_mutex_);
        if (current_status_ != TaskStatus::INITIAL) {
            return;
        }
        current_status_ = TaskStatus::RUNNING;
        result_         = ThreadPool::instance().post_coroutine(
            std::bind(&Task<T>::run_coroutine, this->shared_from_this()));
    }

    virtual boost::asio::awaitable<TaskResult> run_coroutine() = 0;

    TaskStatus get_current_status() {
        std::lock_guard<std::mutex> lock(current_status_mutex_);

        // 如果已经缓存了结果，直接返回当前状态
        if (cached_result_.has_value()) {
            return current_status_;
        }

        // 检查任务是否完成
        auto is_result_valid = result_.valid();
        if (is_result_valid) {
            auto is_result_ready =
                result_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;

            if (is_result_ready) {
                try {
                    auto result    = result_.get();
                    cached_result_ = result;  // 缓存结果，避免重复调用 get()

                    if (result == TaskResult::SUCCESS) {
                        current_status_ = TaskStatus::COMPLETED;
                    } else if (result == TaskResult::FAILURE) {
                        current_status_ = TaskStatus::FAILED;
                    } else if (result == TaskResult::CANCELLED) {
                        current_status_ = TaskStatus::CANCELLED;
                    }
                } catch (...) {
                    cached_result_  = TaskResult::FAILURE;
                    current_status_ = TaskStatus::FAILED;
                }
            }
        }

        return current_status_;
    }

    bool send_input_parameters(const T& value) {
        // 以非阻塞方式尝试发送参数，如果无法发送则返回false
        return input_parameters_channel_.try_send(boost::system::error_code{}, value);
    }

    // 此处不用通过 channel 发送命令，由协程自行轮询，发现 pause 了，就进入暂停状态
    bool pause() { return set_status(TaskStatus::PAUSED); }

    bool resume() {
        std::lock_guard<std::mutex> lock(current_status_mutex_);
        if (current_status_ != TaskStatus::PAUSED) {
            return true;  // 没有牌 pause 状态，认为已经 resume 成功了
        }

        // 先发送命令，成功后再更新状态
        TaskCommand command = TaskCommand::RESUME;
        if (!task_control_channel_.try_send(boost::system::error_code{}, command)) {
            return false;
        }

        // 不能在此处修改状态，应该在 async_wait_for_resume
        // 中确认唤醒了，由唤醒的协程来修改，此处一旦修改，但是可能实际未被唤醒
        return true;
    }

    bool cancel() {
        return task_control_channel_.try_send(boost::system::error_code{}, TaskCommand::CANCEL) &&
               set_status(TaskStatus::CANCELLED);
    }

   protected:
    boost::asio::awaitable<void> async_sleep(std::chrono::milliseconds duration) {
        boost::asio::steady_timer timer(ThreadPool::instance().get_coroutine_io_context(),
                                        duration);
        co_await timer.async_wait(boost::asio::use_awaitable);
    }

    boost::asio::awaitable<tl::expected<T, std::string>> wait_for_input_parameters() {
        auto [e, value] = co_await input_parameters_channel_.async_receive(
            boost::asio::as_tuple(boost::asio::use_awaitable));
        if (e) {
            co_return tl::unexpected(e.message());
        }
        co_return tl::expected<T, std::string>(std::move(value));
    }

    // 等待从暂停状态恢复
    // true: 表示正常从 pause 状态恢复，或者无需等待（不在 pause 状态）
    // false: 表示被异常终止（收到 CANCEL 命令或 channel 错误）
    boost::asio::awaitable<bool> async_wait_for_resume() {
        if (is_paused()) {
            auto [e, command] = co_await task_control_channel_.async_receive(
                boost::asio::as_tuple(boost::asio::use_awaitable));
            if (e) {
                co_return false;  // channel 错误
            }
            if (command == TaskCommand::RESUME) {
                co_return set_status(TaskStatus::RUNNING);
            }
            // 收到其他命令（如 CANCEL）
            co_return false;
        }

        co_return true;  // 不在 pause 状态，无需等待
    }

    bool set_status(TaskStatus status) {
        std::lock_guard<std::mutex> lock(current_status_mutex_);
        if (status == current_status_) {
            return true;
        }

        switch (current_status_) {
            case TaskStatus::INITIAL:
                switch (status) {
                    case TaskStatus::INITIAL:
                        return true;
                    case TaskStatus::RUNNING:
                        current_status_ = TaskStatus::RUNNING;
                        return true;
                    case TaskStatus::CANCELLED:
                        current_status_ = TaskStatus::CANCELLED;
                        return true;
                    case TaskStatus::PAUSED:
                    case TaskStatus::COMPLETED:
                    case TaskStatus::FAILED:
                        return false;  // INITIAL 不能直接转换到这些状态
                    default:
                        return false;
                }
                break;
            case TaskStatus::RUNNING:
                switch (status) {
                    case TaskStatus::INITIAL:
                        return false;  // RUNNING 不能回到 INITIAL
                    case TaskStatus::RUNNING:
                        return true;
                    case TaskStatus::PAUSED:
                        current_status_ = TaskStatus::PAUSED;
                        return true;
                    case TaskStatus::CANCELLED:
                        current_status_ = TaskStatus::CANCELLED;
                        return true;
                    case TaskStatus::COMPLETED:
                        current_status_ = TaskStatus::COMPLETED;
                        return true;
                    case TaskStatus::FAILED:
                        current_status_ = TaskStatus::FAILED;
                        return true;
                    default:
                        return false;
                }
                break;
            case TaskStatus::PAUSED:
                switch (status) {
                    case TaskStatus::INITIAL:
                        return false;
                    case TaskStatus::RUNNING:
                        current_status_ = TaskStatus::RUNNING;
                        return true;
                    case TaskStatus::PAUSED:
                        return true;
                    case TaskStatus::CANCELLED:
                        current_status_ = TaskStatus::CANCELLED;
                        return true;
                    case TaskStatus::COMPLETED:
                        return false;  // PAUSED 不能直接到 COMPLETED
                    case TaskStatus::FAILED:
                        return false;  // PAUSED 不能直接到 FAILED
                    default:
                        return false;
                }
                break;
            case TaskStatus::CANCELLED:
                // CANCELLED 是终态，不能转换到其他状态
                return (status == TaskStatus::CANCELLED);
            case TaskStatus::COMPLETED:
                // COMPLETED 是终态，不能转换到其他状态
                return (status == TaskStatus::COMPLETED);
            case TaskStatus::FAILED:
                // FAILED 是终态，不能转换到其他状态
                return (status == TaskStatus::FAILED);
            default:
                return false;
        }
        return false;
    }

    bool is_paused() {
        std::lock_guard<std::mutex> lock(current_status_mutex_);
        return current_status_ == TaskStatus::PAUSED;
    }

    bool is_cancelled() {
        std::lock_guard<std::mutex> lock(current_status_mutex_);
        return current_status_ == TaskStatus::CANCELLED;
    }

    bool is_completed() {
        std::lock_guard<std::mutex> lock(current_status_mutex_);
        return current_status_ == TaskStatus::COMPLETED;
    }

    bool is_failed() {
        std::lock_guard<std::mutex> lock(current_status_mutex_);
        return current_status_ == TaskStatus::FAILED;
    }

    bool is_running() {
        std::lock_guard<std::mutex> lock(current_status_mutex_);
        return current_status_ == TaskStatus::RUNNING;
    }

    bool is_initial() {
        std::lock_guard<std::mutex> lock(current_status_mutex_);
        return current_status_ == TaskStatus::INITIAL;
    }

   protected:
    std::string name_;

    std::mutex current_status_mutex_;
    TaskStatus current_status_ = TaskStatus::INITIAL;

    std::future<TaskResult>   result_;
    std::optional<TaskResult> cached_result_;  // 缓存结果，避免多次调用 get()

    boost::asio::experimental::channel<InputParamemetersChannelSignature> input_parameters_channel_;

    boost::asio::experimental::channel<TaskControlChannelSignature> task_control_channel_;
};
}  // namespace task
