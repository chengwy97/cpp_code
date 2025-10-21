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
#include <thread>

class printer {
   public:
    printer(boost::asio::io_context& io)
        : io_(io),
          strand_(boost::asio::make_strand(io)),
          timer1_(io, boost::asio::chrono::seconds(1)),
          timer2_(io, boost::asio::chrono::seconds(1)),
          count_(0) {
        timer1_.async_wait(boost::asio::bind_executor(strand_, std::bind(&printer::print1, this)));

        timer2_.async_wait(boost::asio::bind_executor(strand_, std::bind(&printer::print2, this)));
    }

    ~printer() { std::cout << "Final count is " << count_ << std::endl; }

    int count() const { return count_; }

    void print1() {
        if (count_ < 10) {
            std::cout << "Thread ID: " << std::this_thread::get_id() << " Timer 1: " << count_
                      << std::endl;
            ++count_;

            std::this_thread::sleep_for(std::chrono::milliseconds(800));

            timer1_.expires_at(timer1_.expiry() + boost::asio::chrono::seconds(1));

            timer1_.async_wait(
                boost::asio::bind_executor(strand_, std::bind(&printer::print1, this)));
        }
    }

    void print2() {
        if (count_ < 10) {
            std::cout << "Thread ID: " << std::this_thread::get_id() << " Timer 2: " << count_
                      << std::endl;
            ++count_;

            std::this_thread::sleep_for(std::chrono::milliseconds(800));

            timer2_.expires_at(timer2_.expiry() + boost::asio::chrono::seconds(1));

            timer2_.async_wait(
                boost::asio::bind_executor(strand_, std::bind(&printer::print2, this)));
        }
    }

   private:
    boost::asio::io_context&                                    io_;
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    boost::asio::steady_timer                                   timer1_;
    boost::asio::steady_timer                                   timer2_;
    std::atomic<int>                                            count_{0};
};

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    std::cout << "Main thread ID: " << std::this_thread::get_id() << std::endl;

    boost::asio::io_context io;

    auto guard = boost::asio::make_work_guard(io);

    printer     p(io);
    std::thread t1([&] {
        std::cout << "Thread ID: " << std::this_thread::get_id() << " io.run()" << std::endl;
        io.run();
        std::cout << "Thread ID: " << std::this_thread::get_id() << " io.run() end" << std::endl;
    });
    std::thread t2([&] {
        std::cout << "Thread ID: " << std::this_thread::get_id() << " io.run()" << std::endl;
        io.run();
        std::cout << "Thread ID: " << std::this_thread::get_id() << " io.run() end" << std::endl;
    });

    std::this_thread::sleep_for(std::chrono::seconds(10));
    guard.reset();

    // io.run();
    t1.join();
    t2.join();

    std::cout << "Final count is " << p.count() << std::endl;

    return 0;
}
