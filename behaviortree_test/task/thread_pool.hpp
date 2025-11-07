#pragma once

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <future>
#include <memory>
#include <thread>
#include <type_traits>
#include <vector>

namespace task {

// 检测类型是否是 boost::asio::awaitable
template <typename T>
struct is_awaitable : std::false_type {};

template <typename T>
struct is_awaitable<boost::asio::awaitable<T>> : std::true_type {};

template <typename T>
inline constexpr bool is_awaitable_v = is_awaitable<T>::value;

// 从 awaitable 中提取值类型
template <typename T>
struct awaitable_value_type {
    using type = T;
};

template <typename T>
struct awaitable_value_type<boost::asio::awaitable<T>> {
    using type = T;
};

template <typename T>
using awaitable_value_type_t = typename awaitable_value_type<T>::type;

// 基于 asio 的单例线程池
// 包含两个独立的池：一个用于普通函数，一个用于协程
class ThreadPool {
   public:
    // 获取单例实例
    static ThreadPool& instance(size_t normal_threads    = std::thread::hardware_concurrency(),
                                size_t coroutine_threads = std::thread::hardware_concurrency()) {
        static ThreadPool pool(normal_threads, coroutine_threads);
        return pool;
    }

    // 禁止拷贝和移动
    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // 提交普通函数任务，使用 asio post，返回 std::future
    template <class F, class... Args>
    auto post(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using ReturnType = std::invoke_result_t<F, Args...>;

        // 创建 promise 和 future
        auto promise = std::make_shared<std::promise<ReturnType>>();
        auto future  = promise->get_future();

        // 使用 asio post 提交任务到普通函数池的 io_context
        boost::asio::post(
            normal_io_context_,
            [f = std::forward<F>(f), args_tuple = std::make_tuple(std::forward<Args>(args)...),
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

        return future;
    }

    // 提交协程函数任务，使用 co_spawn，返回 std::future
    template <class F, class... Args>
    auto post_coroutine(F&& f, Args&&... args) {
        using RawReturnType = std::invoke_result_t<F, Args...>;
        static_assert(is_awaitable_v<RawReturnType>,
                      "post_coroutine requires a function that returns boost::asio::awaitable");

        using ValueType  = awaitable_value_type_t<RawReturnType>;
        using ReturnType = ValueType;

        // 创建 promise 和 future
        auto promise = std::make_shared<std::promise<ReturnType>>();
        auto future  = promise->get_future();

        // 使用 co_spawn 执行协程到协程池的 io_context
        if constexpr (std::is_void_v<ValueType>) {
            boost::asio::co_spawn(
                coroutine_io_context_,
                [f = std::forward<F>(f), args_tuple = std::make_tuple(std::forward<Args>(
                                             args)...)]() mutable -> boost::asio::awaitable<void> {
                    co_await std::apply(std::move(f), std::move(args_tuple));
                },
                [promise](std::exception_ptr e) mutable {
                    if (e) {
                        promise->set_exception(e);
                    } else {
                        promise->set_value();
                    }
                });
        } else {
            boost::asio::co_spawn(
                coroutine_io_context_,
                [f          = std::forward<F>(f),
                 args_tuple = std::make_tuple(
                     std::forward<Args>(args)...)]() mutable -> boost::asio::awaitable<ReturnType> {
                    co_return co_await std::apply(std::move(f), std::move(args_tuple));
                },
                [promise](std::exception_ptr e, ReturnType result) mutable {
                    if (e) {
                        promise->set_exception(e);
                    } else {
                        promise->set_value(std::move(result));
                    }
                });
        }

        return future;
    }

    // 获取普通函数池的 io_context 引用
    boost::asio::io_context& get_normal_io_context() { return normal_io_context_; }

    // 获取协程池的 io_context 引用
    boost::asio::io_context& get_coroutine_io_context() { return coroutine_io_context_; }

    // 停止线程池
    void stop() {
        normal_io_context_.stop();
        coroutine_io_context_.stop();

        for (auto& thread : normal_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        normal_threads_.clear();

        for (auto& thread : coroutine_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        coroutine_threads_.clear();
    }

   private:
    ThreadPool(size_t normal_threads, size_t coroutine_threads) {
        // 启动普通函数池的工作线程
        for (size_t i = 0; i < normal_threads; ++i) {
            normal_threads_.emplace_back([this]() { normal_io_context_.run(); });
        }

        // 启动协程池的工作线程
        for (size_t i = 0; i < coroutine_threads; ++i) {
            coroutine_threads_.emplace_back([this]() { coroutine_io_context_.run(); });
        }
    }

    ~ThreadPool() { stop(); }

    // 普通函数池
    boost::asio::io_context                                                  normal_io_context_;
    std::vector<std::thread>                                                 normal_threads_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> normal_work_guard_{
        boost::asio::make_work_guard(normal_io_context_)};

    // 协程池
    boost::asio::io_context                                                  coroutine_io_context_;
    std::vector<std::thread>                                                 coroutine_threads_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> coroutine_work_guard_{
        boost::asio::make_work_guard(coroutine_io_context_)};
};

}  // namespace task
