//
// timer.cpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2025 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <functional>
#include <iostream>
#include <boost/asio.hpp>

void print(const std::error_code& /*e*/,
    boost::asio::steady_timer* t, int* count)
{
  if (*count < 5)
  {
    std::cout << *count << std::endl;
    ++(*count);

    t->expires_at(t->expiry() + boost::asio::chrono::seconds(1));
    t->async_wait(std::bind(print,
          boost::asio::placeholders::error, t, count));
  }
}

void test_bind_param(const std::string& str ,int i, double d) {
    std::cout << "str: " << str << ", i: " << i << ", d: " << d << std::endl;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  boost::asio::io_context io;

  int count = 0;
  boost::asio::steady_timer t(io, boost::asio::chrono::seconds(1));
  t.async_wait([&](boost::system::error_code error) {
    print(error, &t, &count);
  });

  io.run();

  std::cout << "Final count is " << count << std::endl;

  std::function<void(int)> f = std::bind(&test_bind_param, "hello", std::placeholders::_1, 2.0);
  f(1);

  return 0;
}