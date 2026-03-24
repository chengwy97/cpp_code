#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/cancellation_condition.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/chrono.hpp>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <boost/thread.hpp>
#include <iostream>

boost::asio::awaitable<std::string> test(int index, int timeout) {
    std::cout << "test " << index << " start" << std::endl;
    co_await boost::asio::steady_timer(co_await boost::asio::this_coro::executor,
                                       boost::asio::chrono::seconds(timeout))
        .async_wait(boost::asio::use_awaitable);
    std::cout << "test " << index << " end" << std::endl;
    co_return "test " + std::to_string(index) + " end";
}

boost::asio::awaitable<void> test_2() {
    std::cout << "test_2 start" << std::endl;
    co_await boost::asio::steady_timer(co_await boost::asio::this_coro::executor,
                                       boost::asio::chrono::seconds(1))
        .async_wait(boost::asio::use_awaitable);
    std::cout << "test_2 end" << std::endl;
    co_return;
}

boost::asio::awaitable<void> test_all() {
    using namespace boost::asio::experimental::awaitable_operators;
    auto result = co_await (test(1, 5) || test(2, 10) || test(3, 15) || test_2());
    if (result.index() == 0) {
        std::cout << "test 1 end" << std::endl;
    } else if (result.index() == 1) {
        std::cout << "test 2 end" << std::endl;
    } else if (result.index() == 2) {
        std::cout << "test 3 end" << std::endl;
    } else if (result.index() == 3) {
        std::cout << "test_2 end" << std::endl;
    }

    auto group1   = (test(1, 5) && test(2, 10));
    auto group2   = (test(3, 15) && test_2());
    auto test_and = co_await (std::move(group1) && std::move(group2));
    std::cout << std::get<0>(test_and) << std::endl;
    std::cout << std::get<1>(test_and) << std::endl;
    std::cout << std::get<2>(test_and) << std::endl;
    co_return;
}

int main() {
    // boost::asio::io_context io;

    // auto                      start = std::chrono::high_resolution_clock::now();
    // boost::asio::steady_timer t(io, boost::asio::chrono::seconds(5));

    // std::this_thread::sleep_for(std::chrono::seconds(4));

    // // 阻塞当前线程
    // t.wait();

    // auto end      = std::chrono::high_resolution_clock::now();
    // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    // std::cout << "执行时间: " << duration.count() << " 毫秒" << std::endl;

    boost::asio::io_context io;
    boost::asio::co_spawn(io, test_all(), boost::asio::detached);
    io.run();
    std::cout << "io.run() end" << std::endl;
    return 0;
}
