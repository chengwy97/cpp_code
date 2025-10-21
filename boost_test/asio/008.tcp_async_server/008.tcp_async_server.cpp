#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

// 注册 signal handler

std::atomic<bool> is_running{true};
// 使用 signal_set 注册 signal handler



using boost::asio::ip::tcp;

boost::asio::awaitable<void> tcp_connect_socket_handler(std::shared_ptr<tcp::socket> socket) {
    try {
        std::cout << "Socket connected!" << std::endl;

        boost::asio::streambuf request_buf;
        co_await boost::asio::async_read_until(*socket, request_buf, "\r\n\r\n",
                                               boost::asio::use_awaitable);
        std::istream request_stream(&request_buf);
        std::string  request_line;
        std::getline(request_stream, request_line);
        std::cout << "Request line: " << request_line << std::endl;

        auto        current_time     = std::chrono::system_clock::now();
        std::time_t current_time_t   = std::chrono::system_clock::to_time_t(current_time);
        std::string current_time_str = std::ctime(&current_time_t);
        current_time_str.pop_back();
        std::cout << "Current time: " << current_time_str << std::endl;

        std::string response =
            "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(current_time_str.size()) +
            "\r\n\r\n" + current_time_str;
        co_await boost::asio::async_write(*socket, boost::asio::buffer(response),
                                          boost::asio::use_awaitable);

        std::cout << "Response sent!" << std::endl;

        socket->close();
        std::cout << "Socket closed!" << std::endl;
    } catch (std::exception& e) {
        std::cerr << "Exception in tcp_connect_socket_handler: " << e.what() << std::endl;
    }
    co_return;
}

boost::asio::awaitable<void> tcp_async_server(boost::asio::io_context& io_context) {
    std::cout << "TCP async server thread ID: " << std::this_thread::get_id() << std::endl;
    try {
        auto          executor = co_await boost::asio::this_coro::executor;
        tcp::acceptor acceptor(executor, tcp::endpoint(tcp::v4(), 8080));

        while (true) {
            std::shared_ptr<tcp::socket> socket = std::make_shared<tcp::socket>(
                co_await acceptor.async_accept(boost::asio::use_awaitable));

            std::cout << "Socket accepted!" << std::endl;

            // 打印 socket 信息
            std::cout << "Socket remote address: " << socket->remote_endpoint().address()
                      << std::endl;
            std::cout << "Socket remote port: " << socket->remote_endpoint().port() << std::endl;
            std::cout << "Socket local address: " << socket->local_endpoint().address()
                      << std::endl;
            std::cout << "Socket local port: " << socket->local_endpoint().port() << std::endl;

            boost::asio::co_spawn(io_context, tcp_connect_socket_handler(std::move(socket)),
                                  boost::asio::detached);
        }
    } catch (std::exception& e) {
        std::cerr << "Exception in server: " << e.what() << std::endl;
    }
    co_return;
}

int main() {
    std::cout << "Main thread ID: " << std::this_thread::get_id() << std::endl;
    boost::asio::io_context io_context;

    auto guard = boost::asio::make_work_guard(io_context);

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&] {
            std::cout << "Thread ID: " << std::this_thread::get_id() << " io.run()" << std::endl;
            io_context.run();
            std::cout << "Thread ID: " << std::this_thread::get_id() << " io.run() end"
                      << std::endl;
        });
    }

    // 启动协程
    boost::asio::co_spawn(io_context, tcp_async_server(io_context), boost::asio::detached);

    io_context.run();
    return 0;
}
