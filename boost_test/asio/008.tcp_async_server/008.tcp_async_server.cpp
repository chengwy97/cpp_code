#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <chrono>
#include <format>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

// 注册 signal handler

std::atomic<bool> is_running{true};
// 使用 signal_set 注册 signal handler

using boost::asio::ip::tcp;

boost::asio::awaitable<void> tcp_connect_socket_handler(std::shared_ptr<tcp::socket> socket) {
    // try {

    try {
        boost::asio::streambuf request_buf;
        co_await boost::asio::async_read_until(*socket, request_buf, "\r\n\r\n",
                                               boost::asio::use_awaitable);
        std::istream request_stream(&request_buf);
        std::string  request_line;
        std::getline(request_stream, request_line);
    } catch (std::exception& e) {
        std::cerr << "Exception 1 in tcp_async_server: " << e.what() << std::endl;
        co_return;
    }

    try {
        std::string current_time_str = "\r\n\r\n";
        try {
            auto        current_time   = std::chrono::system_clock::now();
            std::time_t current_time_t = std::chrono::system_clock::to_time_t(current_time);
            current_time_str           = std::ctime(&current_time_t);
            current_time_str.pop_back();
            current_time_str += "\r\n\r\n";
            // std::cout << "Current time: " << current_time_str << std::endl;
        } catch (std::exception& e) {
            current_time_str = "Error: " + std::string(e.what());
            current_time_str += "\r\n\r\n";
        }

        co_await boost::asio::async_write(*socket, boost::asio::buffer(current_time_str),
                                          boost::asio::use_awaitable);
    } catch (std::exception& e) {
        std::cerr << "Exception 2 in tcp_async_server: " << e.what() << std::endl;
        co_return;
    }
}

boost::asio::awaitable<void> tcp_async_server(boost::asio::io_context& io_context) {
    std::cout << "TCP async server thread ID: " << std::this_thread::get_id() << std::endl;
    // try {
    auto          executor = co_await boost::asio::this_coro::executor;
    tcp::acceptor acceptor(executor);
    acceptor.open(tcp::v4());
    acceptor.set_option(tcp::acceptor::reuse_address(true));
    int optval = 1;
    ::setsockopt(acceptor.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    acceptor.bind(tcp::endpoint(tcp::v4(), 8080));
    acceptor.listen();

    while (true) {
        std::shared_ptr<tcp::socket> socket = std::make_shared<tcp::socket>(
            co_await acceptor.async_accept(boost::asio::use_awaitable));

        boost::asio::co_spawn(io_context.get_executor(),
                              tcp_connect_socket_handler(std::move(socket)), boost::asio::detached);
    }
    // } catch (std::exception& e) {
    //     std::cerr << "Exception in server: " << e.what() << std::endl;
    // }
    co_return;
}

int main() {
    std::cout << "Main thread ID: " << std::this_thread::get_id() << std::endl;
    boost::asio::io_context io_context;

    auto guard = boost::asio::make_work_guard(io_context);

    std::vector<std::thread> threads;
    for (int i = 0; i < 100; ++i) {
        threads.emplace_back([&] {
            std::cout << "Thread ID: " << std::this_thread::get_id() << " io.run()" << std::endl;
            io_context.run();
            std::cout << "Thread ID: " << std::this_thread::get_id() << " io.run() end"
                      << std::endl;
        });
    }

    // 启动协程
    for (int i = 0; i < 8; ++i) {
        boost::asio::co_spawn(io_context, tcp_async_server(io_context), boost::asio::detached);
    }
    io_context.run();
    return 0;
}
