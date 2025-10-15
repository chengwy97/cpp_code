#include <boost/asio/io_context.hpp>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/asio.hpp>

int main() {
    boost::asio::io_context io;

    auto start = std::chrono::high_resolution_clock::now();
    boost::asio::steady_timer t(io, boost::asio::chrono::seconds(5));

    // 异步等待
    t.async_wait([&](const boost::system::error_code& error) {
        if (error) {
            std::cout << "错误: " << error.message() << std::endl;
        } else {
            std::cout << "定时器触发" << std::endl;
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "执行时间: " << duration.count() << " 毫秒" << std::endl;
    });

    std::cout << "等待定时器触发" << std::endl;
    io.run();
    std::cout << "io.run() 结束" << std::endl;
    
    return 0;
}
