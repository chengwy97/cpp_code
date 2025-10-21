#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/streambuf.hpp>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

// 注册 signal handler

std::atomic<bool> is_running{true};
// 使用 signal_set 注册 signal handler

using boost::asio::ip::udp;

boost::asio::awaitable<void> udp_async_server(boost::asio::io_context& io_context, int index) {
    std::cout << "TCP async server thread ID: " << std::this_thread::get_id() << std::endl;
    try {
        auto        executor = co_await boost::asio::this_coro::executor;
        udp::socket socket(executor);
        socket.open(udp::v4());
        socket.set_option(udp::socket::reuse_address(true));
        // socket.set_option(boost::asio::socket_base::reuse_port(true));
        // 手动设置 SO_REUSEPORT
        int optval = 1;
        ::setsockopt(socket.native_handle(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
        socket.bind(udp::endpoint(udp::v4(), 123));
        std::cout << "Socket bound!" << std::endl;

        while (true) {
            std::vector<char>              buf(1024);
            boost::asio::ip::udp::endpoint sender_endpoint;
            size_t                         len = co_await socket.async_receive_from(
                boost::asio::buffer(buf), sender_endpoint, boost::asio::use_awaitable);

            if (std::string(buf.begin(), buf.begin() + len) == "time flush") {
                // std::cout << "Thread ID: " << std::this_thread::get_id() << " Index: " << index
                //           << " Time flush request received!" << std::endl;
                std::string response = "Time flush response!";
                co_await socket.async_send_to(boost::asio::buffer(response), sender_endpoint,
                                              boost::asio::use_awaitable);
                // std::cout << "Thread ID: " << std::this_thread::get_id() << " Index: " << index
                //           << " Time flush response sent!" << std::endl;
                continue;
            }
            // std::cout << "Thread ID: " << std::this_thread::get_id() << " Index: " << index
            //           << " Message received: " << std::string(buf.begin(), buf.begin() + len)
            //           << std::endl;
            // std::cout << "Thread ID: " << std::this_thread::get_id() << " Index: " << index
            //           << " Message received from " << sender_endpoint.address().to_string() <<
            //           ":"
            //           << sender_endpoint.port() << std::endl;
            // std::cout << "Thread ID: " << std::this_thread::get_id() << " Index: " << index
            //           << " Message received!" << std::endl;

            // std::this_thread::sleep_for(std::chrono::milliseconds(100));

            std::string response = "Hello, UDP!";
            co_await socket.async_send_to(boost::asio::buffer(response), sender_endpoint,
                                          boost::asio::use_awaitable);
            // std::cout << "Thread ID: " << std::this_thread::get_id() << " Index: " << index
            //           << " Response sent!" << std::endl;
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
    for (int i = 0; i < 100; ++i) {
        threads.emplace_back([&] {
            std::cout << "Thread ID: " << std::this_thread::get_id() << " io.run()" << std::endl;
            io_context.run();
            std::cout << "Thread ID: " << std::this_thread::get_id() << " io.run() end"
                      << std::endl;
        });
    }

    // 启动协程
    boost::asio::co_spawn(io_context, udp_async_server(io_context, 0), boost::asio::detached);
    boost::asio::co_spawn(io_context, udp_async_server(io_context, 1), boost::asio::detached);
    boost::asio::co_spawn(io_context, udp_async_server(io_context, 2), boost::asio::detached);
    boost::asio::co_spawn(io_context, udp_async_server(io_context, 3), boost::asio::detached);
    boost::asio::co_spawn(io_context, udp_async_server(io_context, 4), boost::asio::detached);
    boost::asio::co_spawn(io_context, udp_async_server(io_context, 5), boost::asio::detached);
    boost::asio::co_spawn(io_context, udp_async_server(io_context, 6), boost::asio::detached);
    boost::asio::co_spawn(io_context, udp_async_server(io_context, 7), boost::asio::detached);

    io_context.run();
    return 0;
}
