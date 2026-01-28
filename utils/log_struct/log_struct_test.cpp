#include "log_struct.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

struct TestData {
    int    a;
    double b;
    double c;
    double d;
    double e;
    double f;
    double g;
    double h;
    double i;
    double j;
};

std::string log_path = "/tmp/test.log";

TEST(LogStructTest, Write) {
    std::filesystem::remove(log_path);
    utils::LogStruct log_struct(log_path, sizeof(TestData), 102400, 1000);

    auto        all_start_time = std::chrono::steady_clock::now();
    std::size_t time_count     = 0;
    for (int i = 0; i < 100000; ++i) {
        TestData test_data;
        test_data.a     = i;
        test_data.b     = i + 1.0;
        test_data.c     = i + 2.0;
        test_data.d     = i + 3.0;
        test_data.e     = i + 4.0;
        test_data.f     = i + 5.0;
        auto start_time = std::chrono::steady_clock::now();
        log_struct.write(reinterpret_cast<const std::byte*>(&test_data));
        auto end_time = std::chrono::steady_clock::now();
        time_count +=
            std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    auto all_end_time = std::chrono::steady_clock::now();
    std::cout << "time_count: " << time_count << std::endl;
    std::cout << "all_time_count: "
              << std::chrono::duration_cast<std::chrono::microseconds>(all_end_time -
                                                                       all_start_time)
                     .count()
              << std::endl;
}

TEST(LogStructTest, Read) {
    std::ifstream ifs(log_path, std::ios::binary);
    if (!ifs.is_open()) {
        FAIL() << "Failed to open log file";
    }

    std::size_t index = 0;
    while (ifs.good()) {
        TestData test_data;
        if (!ifs.read(reinterpret_cast<char*>(&test_data), sizeof(TestData))) {
            std::cout << "Read " << index << " data" << std::endl;
            break;
        }
        EXPECT_EQ(test_data.a, index);
        EXPECT_TRUE(std::abs(test_data.b - (index + 1.0)) < 1e-6);
        EXPECT_TRUE(std::abs(test_data.c - (index + 2.0)) < 1e-6);
        EXPECT_TRUE(std::abs(test_data.d - (index + 3.0)) < 1e-6);
        EXPECT_TRUE(std::abs(test_data.e - (index + 4.0)) < 1e-6);
        EXPECT_TRUE(std::abs(test_data.f - (index + 5.0)) < 1e-6);
        index++;
    }
    ifs.close();
}
