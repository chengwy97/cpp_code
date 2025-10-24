#include <array>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/system/detail/error_code.hpp>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

using boost::asio::as_tuple;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::dynamic_buffer;
using boost::asio::io_context;
using boost::asio::steady_timer;
using boost::asio::experimental::channel;
using boost::asio::ip::tcp;
using namespace boost::asio::buffer_literals;
using namespace std::literals::chrono_literals;

using ChannelSignature = void(boost::system::error_code, int);

std::shared_ptr<channel<ChannelSignature>> channel_ptr = nullptr;

std::array<std::array<bool, 3>, 100> channel_receive_result;

std::atomic_int counter = 0;

awaitable<void> channel_send(int index) {
    boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor);
    timer.expires_after(std::chrono::milliseconds(rand() % 100));
    co_await timer.async_wait();

    channel_receive_result[index][0] = true;
    co_await channel_ptr->async_send({}, 1, boost::asio::use_awaitable);

    channel_receive_result[index][1] = true;

    counter.fetch_add(1);
    std::cout << "Thread ID: " << std::this_thread::get_id() << " Index: " << index
              << " channel_send: " << counter.load() << std::endl;

    // boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor);
    // timer.expires_after(1s);
    // co_await timer.async_wait();

    auto [e, value] =
        co_await channel_ptr->async_receive(boost::asio::as_tuple(boost::asio::use_awaitable));
    if (e) {
        std::cout << "channel_receive error: " << e.message() << std::endl;
    }
    channel_receive_result[index][2] = true;
    co_return;
}

int main() {
    for (auto& row : channel_receive_result) {
        for (auto& cell : row) {
            cell = false;
        }
    }

    std::cout << "Main thread ID: " << std::this_thread::get_id() << std::endl;
    io_context io_context;
    auto       guard = boost::asio::make_work_guard(io_context);

    std::vector<std::thread> threads;
    for (int i = 0; i < 200; ++i) {
        threads.emplace_back([&] {
            std::cout << "Thread ID: " << std::this_thread::get_id() << " io.run()" << std::endl;
            io_context.run();
            std::cout << "Thread ID: " << std::this_thread::get_id() << " io.run() end"
                      << std::endl;
        });
    }

    channel_ptr = std::make_shared<channel<ChannelSignature>>(io_context.get_executor(), 1);

    for (int i = 0; i < 100; ++i) {
        co_spawn(io_context, channel_send(i), detached);
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    while (counter < 100 && std::chrono::high_resolution_clock::now() - start_time < 100s) {
        std::this_thread::sleep_for(1s);
    }

    if (counter < 100) {
        return -1;
    }

    std::cout << "Counter: " << counter.load() << std::endl;
    for (int i = 0; i < 100; ++i) {
        std::cout << "Channel receive result[" << i << "][0]: " << channel_receive_result[i][0]
                  << " Channel receive result[" << i << "][1]: " << channel_receive_result[i][1]
                  << " Channel receive result[" << i << "][2]: " << channel_receive_result[i][2]
                  << std::endl;
    }

    channel_ptr->close();
    std::this_thread::sleep_for(1s);

    guard.reset();
    for (auto& thread : threads) {
        thread.join();
    }
    return 0;
}
