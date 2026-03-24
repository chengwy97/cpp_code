#include "thread_pool.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <random>
#include <thread>

#if defined(__cpp_lib_coroutine) && __cpp_lib_coroutine >= 201902L
#include <coroutine>
#endif

using namespace utils;

TEST(ThreadPoolTest, PostSimpleTask) {
    ThreadPool::set_running_thread_count(2);
    auto& pool = ThreadPool::instance();

    auto future = pool.post([](int a, int b) { return a + b; }, 2, 3);
    EXPECT_EQ(future->get(), 5);
}

TEST(ThreadPoolTest, PostVoidTask) {
    ThreadPool::set_running_thread_count(2);
    auto& pool = ThreadPool::instance();

    std::atomic<int> value{0};
    auto             future = pool.post([&value]() { value.store(42, std::memory_order_relaxed); });
    future->get();
    EXPECT_EQ(value.load(), 42);
}

TEST(ThreadPoolTest, AddLongRunningTask) {
    ThreadPool::set_running_thread_count(2);
    auto& pool = ThreadPool::instance();

    std::atomic<int>                   count{0};
    std::random_device                 rd;
    std::mt19937                       gen(rd());
    std::uniform_int_distribution<int> dis(1, 100);
    auto                               test_task = [&count, &gen, &dis](std::stop_token st) {
        while (!st.stop_requested()) {
            count.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));
            break;
        }
    };

    auto test_thread_number = std::thread::hardware_concurrency();
    for (int i = 0; i < test_thread_number; ++i) {
        pool.addLongRunningTask("test_task", test_task);
    }

    // 等待任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(count.load(), test_thread_number);
}

#if defined(__cpp_lib_coroutine) && __cpp_lib_coroutine >= 201902L
boost::asio::awaitable<int> post_coro_task() {
    co_return 123;
}

TEST(ThreadPoolTest, PostCoroTask) {
    ThreadPool::set_running_thread_count(2);
    auto& pool = ThreadPool::instance();

    auto future = pool.post_coro(post_coro_task);

    EXPECT_EQ(future->get(), 123);
}
#endif

TEST(ThreadPoolTest, StopPreventsNewTasks) {
    ThreadPool::set_running_thread_count(2);
    auto& pool = ThreadPool::instance();

    pool.stop();

    auto future = pool.post([]() { return 1; });
    // stop 后依然可以 post，但不会执行，future 会阻塞或者抛异常，这里我们用 try/catch
    EXPECT_FALSE(future.has_value());

    bool success = pool.addLongRunningTask("task_after_stop", [](std::stop_token) {});
    EXPECT_FALSE(success);
}
