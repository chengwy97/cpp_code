#pragma once

#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <gsl/gsl>
#include <iostream>
#include <list>
#include <optional>
#include <stop_token>
#include <thread>
#include <utility>

#if defined(__cpp_lib_coroutine) && __cpp_lib_coroutine >= 201902L
#include <coroutine>
#endif

#include "boost/asio/io_context.hpp"

namespace utils {
class ThreadPool {
    struct PrivateTag {};

    inline static std::size_t g_default_thread_count_ = std::thread::hardware_concurrency();

   public:
    static void set_running_thread_count(std::size_t thread_count) {
        Expects(thread_count > 0);
        g_default_thread_count_ = thread_count;
    }

    /**
     * @brief 获取单例实例,如果想要设置线程数,必须先调用 set_running_thread_count 函数设置线程数
     *        注意线程池的生命周期
     * @return 单例实例
     */
    static ThreadPool& instance() {
        static ThreadPool pool(PrivateTag{}, g_default_thread_count_);
        return pool;
    }

    ThreadPool()                             = delete;
    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool(ThreadPool&&)                 = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&)      = delete;

    ThreadPool(PrivateTag, std::size_t thread_count = g_default_thread_count_) {
        Expects(thread_count > 0);

        small_task_workers_.reserve(thread_count);
        for (std::size_t i = 0; i < thread_count; ++i) {
            small_task_workers_.emplace_back([this]() {
                pthread_setname_np(pthread_self(), "small_task_worker");
                io_context_.run();
            });
        }
    }
    ~ThreadPool() { stop(); }

    boost::asio::io_context& getIoContext() { return io_context_; }

    bool addLongRunningTask(std::string_view                     thread_name,
                            std::function<void(std::stop_token)> task) {
        if (!is_running_.load(std::memory_order_relaxed)) {
            return false;
        }

        std::lock_guard<std::mutex> lock(long_running_workers_mutex_);
        auto                        stop_source = std::make_unique<std::stop_source>();
        auto                        stop_token  = stop_source->get_token();
        long_running_workers_.emplace_back(
            std::make_pair(std::move(stop_source),
                           std::make_unique<std::jthread>([this, thread_name, stop_token, task]() {
                               execute_task(thread_name, stop_token, task);
                           })));
        return true;
    }

    // 提交函数任务，使用 asio post，返回 std::future
    // 1. 普通 future 版本
    template <class F, class... Args>
    auto post(F&& f,
              Args&&... args) -> std::optional<std::future<std::invoke_result_t<F, Args...>>> {
        using ReturnType = std::invoke_result_t<F, Args...>;

        if (!is_running_.load(std::memory_order_relaxed)) {
            return std::nullopt;
        }

        // 创建 promise 和 future
        auto promise = std::make_shared<std::promise<ReturnType>>();
        auto future  = promise->get_future();

        // 使用 asio post 提交任务到 io_context
        boost::asio::post(io_context_, [f          = std::forward<F>(f),
                                        args_tuple = std::make_tuple(std::forward<Args>(args)...),
                                        promise]() mutable {
            try {
                if constexpr (std::is_void_v<ReturnType>) {
                    std::apply(std::move(f), std::move(args_tuple));
                    promise->set_value();
                } else {
                    promise->set_value(std::apply(std::move(f), std::move(args_tuple)));
                }
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        });

        return std::make_optional<std::future<ReturnType>>(std::move(future));
    }

#if defined(__cpp_lib_coroutine) && __cpp_lib_coroutine >= 201902L
    template <class F, class... Args>
    auto post_coro(F&& f, Args&&... args)
        -> std::optional<std::future<typename std::invoke_result_t<F, Args...>::value_type>> {
        using AwaitableType = std::invoke_result_t<F, Args...>;
        using ReturnType    = typename AwaitableType::value_type;  // 协程返回 T

        if (!is_running_.load(std::memory_order_relaxed)) {
            return std::nullopt;
        }

        auto promise = std::make_shared<std::promise<ReturnType>>();
        auto future  = promise->get_future();

        boost::asio::post(io_context_,
                          [this, promise, f = std::forward<F>(f),
                           args_tuple = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                              try {
                                  auto task = std::apply(std::move(f), std::move(args_tuple));
                                  auto fut  = boost::asio::co_spawn(io_context_, std::move(task),
                                                                    boost::asio::use_future);
                                  if constexpr (std::is_void_v<ReturnType>) {
                                      fut.get();
                                      promise->set_value();
                                  } else {
                                      auto result = fut.get();
                                      promise->set_value(std::move(result));
                                  }
                              } catch (...) {
                                  promise->set_exception(std::current_exception());
                              }
                          });

        return std::make_optional<std::future<ReturnType>>(std::move(future));
    }
#endif

    void stop() {
        bool expected = true;
        if (!is_running_.compare_exchange_strong(expected, false)) {
            return;
        }

        io_context_.stop();
        for (auto& thread : small_task_workers_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        small_task_workers_.clear();

        std::lock_guard<std::mutex> lock(long_running_workers_mutex_);
        for (auto& [stop_source, thread] : long_running_workers_) {
            stop_source->request_stop();
            thread->join();
        }

        long_running_workers_.clear();
    }

   private:
    std::atomic<bool> is_running_ = true;

    std::mutex long_running_workers_mutex_;
    std::list<std::pair<std::unique_ptr<std::stop_source>, std::unique_ptr<std::jthread>>>
        long_running_workers_{};

    boost::asio::io_context                                                  io_context_;
    std::vector<std::jthread>                                                small_task_workers_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> small_task_work_guard_{
        boost::asio::make_work_guard(io_context_)};

    void execute_task(std::string_view thread_name, std::stop_token stop_token,
                      std::function<void(std::stop_token)> task) {
        // 设置线程名称
        pthread_setname_np(pthread_self(), thread_name.data());
        // 执行任务
        task(stop_token);
    }
};
}  // namespace utils
