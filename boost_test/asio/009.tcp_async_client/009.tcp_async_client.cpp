#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/read_until.hpp>
#include <iostream>
#include <thread>

using boost::asio::ip::tcp;

std::atomic_size_t g_count_time_great_100ms = 0;
std::atomic_size_t g_count_session          = 0;

boost::asio::awaitable<void> client_session(boost::asio::io_context&    io_context,
                                            tcp::resolver::results_type endpoints) {
    // C++20 协程让异步操作看起来像同步操作
    // 注意：这里没有回调函数，而是直接使用 co_await
    // std::cout << "Client session thread ID: " << std::this_thread::get_id() << std::endl;

    try {
        auto        begin_time = std::chrono::steady_clock::now();
        auto        executor   = io_context.get_executor();
        tcp::socket socket(executor);

        // co_await 异步连接操作
        co_await boost::asio::async_connect(socket, endpoints, boost::asio::use_awaitable);

        if (socket.is_open()) {
            // std::cout << "Connection successful!" << std::endl;
        } else {
            std::cerr << "Connection failed!" << std::endl;
            g_count_session.fetch_add(1);
            co_return;
        }

        // 打印 socket 信息
        // std::cout << "Socket remote address: " << socket.remote_endpoint().address() <<
        // std::endl; std::cout << "Socket remote port: " << socket.remote_endpoint().port() <<
        // std::endl; std::cout << "Socket local address: " << socket.local_endpoint().address() <<
        // std::endl; std::cout << "Socket local port: " << socket.local_endpoint().port() <<
        // std::endl;

        // std::cout << "Sending request..." << std::endl;
        std::string request = "GET / HTTP/1.1\r\nHost: www.example.com\r\n\r\n";
        co_await boost::asio::async_write(socket, boost::asio::buffer(request),
                                          boost::asio::use_awaitable);

        // std::cout << "Request sent!" << std::endl;

        // std::cout << "Reading response..." << std::endl;
        boost::asio::streambuf buf;
        co_await boost::asio::async_read_until(socket, buf, "\r\n\r\n", boost::asio::use_awaitable);

        // std::cout << "Response: " << std::endl;
        // std::istream is(&buf);
        // std::string  line;
        // while (std::getline(is, line)) {
        //     std::cout << line << std::endl;
        // }
        // std::cout << "Response read!" << std::endl;
        auto end_time = std::chrono::steady_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - begin_time);
        if (duration.count() > 100) {
            g_count_time_great_100ms.fetch_add(1);
        }
        g_count_session.fetch_add(1);
        // socket.close();
        co_return;
    } catch (std::exception& e) {
        std::cerr << "Exception in client: " << e.what() << std::endl;
        g_count_session.fetch_add(1);
        co_return;
    }
}

int main() {
    std::cout << "Main thread ID: " << std::this_thread::get_id() << std::endl;
    boost::asio::io_context io_context;
    auto                    guard = boost::asio::make_work_guard(io_context);

    std::vector<std::thread> threads;
    for (int i = 0; i < 100; ++i) {
        threads.emplace_back([&] {
            // std::cout << "Thread ID: " << std::this_thread::get_id() << " io.run()" << std::endl;
            io_context.run();
            // std::cout << "Thread ID: " << std::this_thread::get_id() << " io.run() end"
            //           << std::endl;
        });
    }

    // 给 endpoints 赋值为本地地址 127.0.0.1 8080
    tcp::resolver::results_type endpoints = tcp::resolver(io_context).resolve("127.0.0.1", "8080");

    // 启动协程
    for (int i = 0; i < 10000; ++i) {
        boost::asio::co_spawn(io_context, client_session(io_context, endpoints),
                              boost::asio::detached);
    }

    while (g_count_session.load() < 10000) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << std::format("g_count_time_great_100ms: {}\n", g_count_time_great_100ms.load());

    guard.reset();
    for (auto& thread : threads) {
        thread.join();
    }
    return 0;
}
