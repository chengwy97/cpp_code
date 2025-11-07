#include <iostream>
#include <logging/log_def.hpp>
#include <task.hpp>
#include <thread_pool.hpp>

void print_hello() {
    UTILS_LOG_INFO("Hello, world!");
}

boost::asio::awaitable<void> print_hello_async() {
    co_await boost::asio::this_coro::executor;
    UTILS_LOG_INFO("Hello, world!");
}

int main() {
    UTILS_LOG_INIT("test.log", "info");

    auto& thread_pool       = task::ThreadPool::instance();
    auto  sync_print_hello  = thread_pool.post(print_hello);
    auto  async_print_hello = thread_pool.post_coroutine(print_hello_async);

    sync_print_hello.get();
    async_print_hello.get();

    UTILS_LOG_FLUSH();
    return 0;
}
