#include <atomic>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/read_until.hpp>
#include <iostream>
#include <thread>

using boost::asio::ip::udp;
namespace this_coro = boost::asio::this_coro;
using namespace boost::asio::experimental::awaitable_operators;

std::atomic_size_t timeout_count = 0;

boost::asio::awaitable<void> receive_with_timeout(udp::socket&         socket,
                                                  std::chrono::seconds timeout) {
    auto executor = co_await this_coro::executor;

    std::vector<char>         buf(1024);
    udp::endpoint             sender_endpoint;
    boost::asio::steady_timer timer(executor);
    timer.expires_after(timeout);

    boost::system::error_code ec_recv, ec_timer;
    // 两个异步操作竞争：
    // (1) async_receive_from
    // (2) timer.async_wait
    // 两个异步操作竞争
    auto result = co_await (
        socket.async_receive_from(
            boost::asio::buffer(buf), sender_endpoint,
            boost::asio::redirect_error(boost::asio::use_awaitable, ec_recv)) ||
        timer.async_wait(boost::asio::redirect_error(boost::asio::use_awaitable, ec_timer)));

    // result 是 std::variant<unsigned long, std::monostate>
    if (std::holds_alternative<unsigned long>(result)) {
        // ✅ 第一个操作完成（收到数据）
        timer.cancel();
        std::size_t n = std::get<unsigned long>(result);  // 收到的字节数
        if (!ec_recv) {
            // std::cout << "Received " << n << " bytes\n";
        }
        // std::cout << "Received: " << std::string(buf.begin(), buf.begin() + n) << std::endl;
    } else {
        // ❌ 第二个操作完成（超时）
        socket.cancel();
        std::cout << "Receive timeout\n";
        timeout_count++;
    }

    co_return;
}

std::atomic_size_t counter = 0;

boost::asio::awaitable<void> time_flush_request(boost::asio::io_context& io_context,
                                                udp::endpoint            receiver_endpoint) {
    try {
        // std::cout << "Time flush request thread ID: " << std::this_thread::get_id() << std::endl;
        auto        executor = io_context.get_executor();
        udp::socket socket(executor);
        socket.open(udp::v4());

        // std::cout << "Sending time flush request..." << std::endl;
        std::string request = "time flush";
        size_t len = co_await socket.async_send_to(boost::asio::buffer(request), receiver_endpoint,
                                                   boost::asio::use_awaitable);
        // std::cout << "Time flush request sent! Length: " << len << std::endl;

        // std::cout << "Reading response..." << std::endl;
        // std::vector<char>              buf(102400);
        // boost::asio::ip::udp::endpoint sender_endpoint;
        // len = co_await socket.async_receive_from(boost::asio::buffer(buf), sender_endpoint,
        //                                          boost::asio::use_awaitable);
        co_await receive_with_timeout(socket, std::chrono::seconds(3));
        // std::cout << "Response: " << std::string(buf.begin(), buf.begin() + len) << std::endl;
        // std::cout << "Response read!" << std::endl;

        counter++;

        socket.close();
        // std::cout << "Socket closed!" << std::endl;

        co_return;
    } catch (std::exception& e) {
        std::cerr << "Exception in time flush request: " << e.what() << std::endl;
        counter++;
        co_return;
    }
}

boost::asio::awaitable<void> hello_request(boost::asio::io_context& io_context,
                                           udp::endpoint            receiver_endpoint) {
    try {
        // std::cout << "Hello request thread ID: " << std::this_thread::get_id() << std::endl;
        auto        executor = io_context.get_executor();
        udp::socket socket(executor);
        socket.open(udp::v4());

        // std::cout << "Sending hello request..." << std::endl;
        std::string request = "Hello, UDP!";
        size_t len = co_await socket.async_send_to(boost::asio::buffer(request), receiver_endpoint,
                                                   boost::asio::use_awaitable);
        // std::cout << "Hello request sent! Length: " << len << std::endl;

        // std::cout << "Reading response..." << std::endl;
        // std::vector<char>              buf(102400);
        // boost::asio::ip::udp::endpoint sender_endpoint;
        // len = co_await socket.async_receive_from(boost::asio::buffer(buf), sender_endpoint,
        //                                          boost::asio::use_awaitable);
        co_await receive_with_timeout(socket, std::chrono::seconds(3));
        // std::cout << "Response: " << std::string(buf.begin(), buf.begin() + len) << std::endl;
        // std::cout << "Response read!" << std::endl;

        counter++;

        socket.close();
        // std::cout << "Socket closed!" << std::endl;

        co_return;
    } catch (std::exception& e) {
        std::cerr << "Exception in hello request: " << e.what() << std::endl;
        counter++;
        co_return;
    }
    co_return;
}

boost::asio::awaitable<void> client_session(boost::asio::io_context& io_context,
                                            udp::endpoint            receiver_endpoint) {
    // C++20 协程让异步操作看起来像同步操作
    // 注意：这里没有回调函数，而是直接使用 co_await
    std::cout << "Client session thread ID: " << std::this_thread::get_id() << std::endl;

    std::cout << "Receiver endpoint: " << receiver_endpoint.address().to_string() << ":"
              << receiver_endpoint.port() << std::endl;
    try {
        for (size_t i = 0; i < 100000; i++) {
            co_await boost::asio::post(io_context, boost::asio::use_awaitable);
            auto random_number = std::rand() % 2;
            if (random_number == 0) {
                boost::asio::co_spawn(io_context, time_flush_request(io_context, receiver_endpoint),
                                      boost::asio::detached);
            } else {
                boost::asio::co_spawn(io_context, hello_request(io_context, receiver_endpoint),
                                      boost::asio::detached);
            }
        }

        int last_counter = 0;
        // 等待所有协程完成, 即 counter 等于 1000000
        while (counter != 10000) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            std::cout << "Counter: " << counter << " - " << last_counter << std::endl;
            last_counter = counter;
        }
        std::cout << "All requests completed! Counter: " << counter << std::endl;
        std::cout << "Timeout count: " << timeout_count << std::endl;
    } catch (std::exception& e) {
        std::cerr << "Exception in client: " << e.what() << std::endl;
    }

    co_return;
}

int main() {
    std::cout << "Main thread ID: " << std::this_thread::get_id() << std::endl;
    boost::asio::io_context io_context;

    auto guard = boost::asio::make_work_guard(io_context);

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&] { io_context.run(); });
    }

    boost::asio::ip::address ip_address = boost::asio::ip::make_address_v4("127.0.0.1");

    // 2. 构造 udp::endpoint 对象（结合 IP 地址和端口号 123）
    udp::endpoint receiver_endpoint(ip_address, 123);

    // 启动协程
    boost::asio::co_spawn(io_context, client_session(io_context, receiver_endpoint),
                          boost::asio::detached);

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    guard.reset();

    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "All threads joined!" << std::endl;

    return 0;
}
