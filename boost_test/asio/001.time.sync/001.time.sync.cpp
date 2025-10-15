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

    std::this_thread::sleep_for(std::chrono::seconds(4));

    // 阻塞当前线程
    t.wait();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "执行时间: " << duration.count() << " 毫秒" << std::endl;
    
    return 0;
}
