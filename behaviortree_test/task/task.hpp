#pragma once

#include <atomic>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/system/detail/error_code.hpp>
#include <chrono>
#include <future>
#include <logging/log_def.hpp>
#include <mutex>
#include <optional>
#include <string>
#include <tl/tl_expected.hpp>
#include <tuple>
#include <variant>

#include "thread_pool.hpp"

namespace task {

// 用户下发的控制命令
enum class TaskCommand {
    CANCEL,  // 取消任务
    PAUSE,   // 暂停任务
    RESUME,  // 恢复任务
};

inline std::string to_string(TaskCommand command) {
    switch (command) {
        case TaskCommand::CANCEL:
            return "CANCEL";
        case TaskCommand::PAUSE:
            return "PAUSE";
        case TaskCommand::RESUME:
            return "RESUME";
    }
}

// 任务反馈的实际状态
enum class TaskStatus {
    INITIAL,    // 任务初始状态
    RUNNING,    // 任务运行中
    COMPLETED,  // 任务完成
    FAILED,     // 任务失败
    CANCELLED,  // 任务已取消
    PAUSED,     // 任务已暂停
};

inline std::string to_string(TaskStatus status) {
    switch (status) {
        case TaskStatus::INITIAL:
            return "INITIAL";
        case TaskStatus::RUNNING:
            return "RUNNING";
        case TaskStatus::COMPLETED:
            return "COMPLETED";
        case TaskStatus::FAILED:
            return "FAILED";
        case TaskStatus::CANCELLED:
            return "CANCELLED";
        case TaskStatus::PAUSED:
            return "PAUSED";
        default:
            return "UNKNOWN";
    }
}

enum class TaskResult {
    SUCCESS,
    FAILURE,
    CANCELLED,
};

inline std::string to_string(TaskResult result) {
    switch (result) {
        case TaskResult::SUCCESS:
            return "SUCCESS";
        case TaskResult::FAILURE:
            return "FAILURE";
        case TaskResult::CANCELLED:
            return "CANCELLED";
        default:
            return "UNKNOWN";
    }
}

/**
 * run_coroutine: 运行协程 // 用户实现
 * get_current_status: 获取当前状态 // 用户调用
 * send_input_parameters: 发送输入参数 // 用户调用
 * pause: 暂停任务 // 用户调用
 * resume: 恢复任务 // 用户调用
 * cancel: 取消任务 // 用户调用
 * async_sleep: 异步睡眠 // 用户可以在 run_coroutine 中使用
 * wait_for_input_parameters: 等待输入参数 // 用户可以在 run_coroutine 中使用
 * async_wait_for_resume: 等待从暂停状态恢复 // 用户可以在 run_coroutine 中使用
 * is_paused: 是否暂停 // 用户可以在 run_coroutine 中使用
 * is_cancelled: 是否取消 // 用户可以在 run_coroutine 中使用
 * is_completed: 是否完成 // 用户可以在 run_coroutine 中使用
 * is_failed: 是否失败 // 用户可以在 run_coroutine 中使用
 * is_running: 是否运行 // 用户可以在 run_coroutine 中使用
 * is_initial: 是否初始化 // 用户可以在 run_coroutine 中使用
 */

template <typename T>
class Task : public std::enable_shared_from_this<Task<T>> {
   public:
    using InputParamemetersChannelSignature = void(boost::system::error_code, T);
    using TaskControlChannelSignature       = void(boost::system::error_code, TaskCommand);

    Task(const std::string& name)
        : name_(name),
          input_parameters_channel_(
              ThreadPool::instance().get_coroutine_io_context().get_executor(), 1),
          task_control_channel_(ThreadPool::instance().get_coroutine_io_context().get_executor(),
                                1) {}
    virtual ~Task() {
        while (!cancel()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        input_parameters_channel_.close();
        task_control_channel_.close();
    }

    bool run() {
        std::lock_guard<std::mutex> lock(current_status_mutex_);
        if (current_status_ != TaskStatus::INITIAL) {
            UTILS_LOG_ERROR("Task {} run failed, current status is not INITIAL", name_);
            return false;
        }

        current_status_ = TaskStatus::RUNNING;
        result_         = ThreadPool::instance().post_coroutine(
            std::bind(&Task<T>::run_coroutine, this->shared_from_this()));
        return true;
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
    bool pause() {
        UTILS_LOG_DEBUG("Task {} pause", name_);

        std::lock_guard<std::mutex> lock(current_status_mutex_);
        if (current_status_ != TaskStatus::RUNNING && current_status_ != TaskStatus::PAUSED) {
            return false;
        }

        if (!task_control_channel_.try_send(boost::system::error_code{}, TaskCommand::PAUSE)) {
            return false;
        }

        current_status_ = TaskStatus::PAUSED;
        return true;
    }

    bool resume() {
        UTILS_LOG_DEBUG("Task {} resume", name_);
        // 先发送命令，成功后再更新状态
        TaskCommand command = TaskCommand::RESUME;
        if (!task_control_channel_.try_send(boost::system::error_code{}, command)) {
            UTILS_LOG_ERROR("Task {} resume failed", name_);
            return false;
        }

        // 不能在此处修改状态，应该在 async_wait_for_resume
        // 中确认唤醒了，由唤醒的协程来修改，此处一旦修改，但是可能实际未被唤醒
        UTILS_LOG_DEBUG("Task {} resume success", name_);
        return true;
    }

    // cancel 一旦被调用，任务状态已经发生变化，就算返回
    // false，任务内部状态也发生变化了，后续只能再重复调用 cancel 不能再认为任务状态未取消成功
    bool cancel() {
        std::lock_guard<std::mutex> lock(current_status_mutex_);
        if (current_status_ == TaskStatus::COMPLETED || current_status_ == TaskStatus::FAILED) {
            return true;
        }

        if (current_status_ != TaskStatus::RUNNING && current_status_ != TaskStatus::PAUSED &&
            current_status_ != TaskStatus::CANCELLED) {
            return false;
        }

        if (!task_control_channel_.try_send(boost::system::error_code{}, TaskCommand::CANCEL)) {
            return false;
        }

        current_status_ = TaskStatus::CANCELLED;
        return true;
    }

   protected:
    boost::asio::awaitable<void> async_sleep(std::chrono::milliseconds duration) {
        boost::asio::steady_timer timer(ThreadPool::instance().get_coroutine_io_context(),
                                        duration);
        co_await timer.async_wait(boost::asio::use_awaitable);
    }

    // 等待输入参数，同时可以响应控制命令
    // - 如果收到 CANCEL 命令，返回错误
    // - 如果收到 PAUSE 命令，进入暂停状态并等待恢复
    // - 如果收到 RESUME 命令，继续等待输入参数
    boost::asio::awaitable<tl::expected<T, std::string>> wait_for_input_parameters() {
        using namespace boost::asio::experimental::awaitable_operators;

        UTILS_LOG_DEBUG("Task {} is waiting for input parameters", name_);
        // 同时等待输入参数和控制命令
        auto result = co_await (
            input_parameters_channel_.async_receive(
                boost::asio::as_tuple(boost::asio::use_awaitable)) ||
            task_control_channel_.async_receive(boost::asio::as_tuple(boost::asio::use_awaitable)));

        // result 是 std::variant<std::tuple<error_code, T>, std::tuple<error_code,
        // TaskCommand>>
        if (std::holds_alternative<std::tuple<boost::system::error_code, T>>(result)) {
            // 收到输入参数
            UTILS_LOG_DEBUG("Task {} received input parameters", name_);
            auto [e, value] = std::get<std::tuple<boost::system::error_code, T>>(result);
            if (e) {
                UTILS_LOG_ERROR("Task {} received input parameters error: {}", name_, e.message());
                co_return tl::unexpected(e.message());
            }
            UTILS_LOG_DEBUG("Task {} received input parameters", name_);
            co_return tl::expected<T, std::string>(std::move(value));
        } else {
            // 收到控制命令
            UTILS_LOG_DEBUG("Task {} received control command", name_);
            auto [e, command] =
                std::get<std::tuple<boost::system::error_code, TaskCommand>>(result);
            if (e) {
                UTILS_LOG_ERROR("Task {} received control command error: {}", name_, e.message());
                co_return tl::unexpected(e.message());
            }

            // 处理控制命令
            if (command == TaskCommand::CANCEL) {
                UTILS_LOG_DEBUG("Task {} received CANCEL command, cancelling", name_);
                co_return tl::unexpected(
                    fmt::format("Task {} cancelled while waiting for input parameters", name_));
            } else if (command == TaskCommand::PAUSE) {
                UTILS_LOG_DEBUG("Task {} received PAUSE command, pausing", name_);
                co_return tl::unexpected(
                    fmt::format("Task {} paused while waiting for input parameters", name_));
            } else if (command == TaskCommand::RESUME) {
                UTILS_LOG_DEBUG("Task {} received RESUME command, resuming", name_);
                co_return tl::unexpected(
                    fmt::format("Task {} resumed while waiting for input parameters", name_));
            }
        }

        co_return tl::unexpected(
            fmt::format("Task {} unknown error while waiting for input parameters", name_));
    }

    // 等待从暂停状态恢复
    // true: 表示正常从 pause 状态恢复，或者无需等待（不在 pause 状态）
    // false: 表示被异常终止（收到 CANCEL 命令或 channel 错误）
    boost::asio::awaitable<bool> async_wait_for_resume() {
        UTILS_LOG_DEBUG("Task {} is waiting for resume", name_);

        while (is_paused()) {
            UTILS_LOG_DEBUG("Task {} is paused, waiting for resume", name_);

            auto [e, command] = co_await task_control_channel_.async_receive(
                boost::asio::as_tuple(boost::asio::use_awaitable));
            if (e) {
                UTILS_LOG_ERROR("Task {} async_wait_for_resume error: {}", name_, e.message());
                co_return false;  // channel 错误
            }
            if (command == TaskCommand::RESUME) {
                UTILS_LOG_DEBUG("Task {} received RESUME command, resuming", name_);
                co_return set_status(TaskStatus::RUNNING);
            }

            if (command == TaskCommand::PAUSE) {
                UTILS_LOG_DEBUG("Task {} received PAUSE command, pausing", name_);
                continue;
            }

            // 收到其他命令（如 CANCEL）
            UTILS_LOG_ERROR("Task {} received unknown command: {}", name_, to_string(command));
            co_return false;
        }

        UTILS_LOG_DEBUG("Task {} is not paused, no need to wait for resume", name_);
        co_return true;  // 不在 pause 状态，无需等待
    }

    bool set_status(TaskStatus status) {
        UTILS_LOG_DEBUG("Task {} set_status from {} to {}", name_, to_string(current_status_),
                        to_string(status));

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
