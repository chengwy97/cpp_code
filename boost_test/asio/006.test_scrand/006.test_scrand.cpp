//
// timer.cpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2025 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio.hpp>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_set>

std::mutex cout_mutex;

std::mutex                          mani_thread_ids_mutex;
std::unordered_set<std::thread::id> mani_thread_ids;

std::unordered_set<std::thread::id> t3_thread_ids;
std::mutex                          t3_thread_ids_mutex;

void print(boost::asio::strand<boost::asio::io_context::executor_type> strand, int n) {
    boost::asio::post(strand, [n] {
        std::lock_guard<std::mutex> lock(mani_thread_ids_mutex);
        mani_thread_ids.insert(std::this_thread::get_id());
    });
}

int main() {
    boost::asio::io_context io;
    auto                    strand = boost::asio::make_strand(io);

    auto guard = boost::asio::make_work_guard(io);

    std::thread t1([&] { io.run(); });
    std::thread t2([&] { io.run(); });
    std::thread t4([&] { io.run(); });
    std::thread t5([&] { io.run(); });

    std::thread t3([&] {
        for (int i = 0; i < 1000; ++i)
            boost::asio::post(io, [&, i]() {
                std::lock_guard<std::mutex> lock(t3_thread_ids_mutex);
                t3_thread_ids.insert(std::this_thread::get_id());
            });
    });

    for (int i = 0; i < 1000; ++i) print(strand, i);

    std::this_thread::sleep_for(std::chrono::seconds(10));
    guard.reset();

    t3.join();
    t1.join();
    t2.join();
    t4.join();
    t5.join();

    std::cout << "mani_thread_ids: ";
    for (auto& id : mani_thread_ids) {
        std::cout << id << " ";
    }
    std::cout << std::endl;

    std::cout << "t3_thread_ids: ";
    for (auto& id : t3_thread_ids) {
        std::cout << id << " ";
    }
    std::cout << std::endl;

    return 0;
}
