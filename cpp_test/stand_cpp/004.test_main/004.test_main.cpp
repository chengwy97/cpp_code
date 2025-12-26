#include "version/version.hpp"

#include <iostream>
#include <variant>
#include <string>
#include <stdexcept>
#include <string_view>
#include <thread>

struct Test {
    int a;
    double b;
    std::string c;
};

int main(int argc, char* argv[]) {
    std::cout << "Git commit ID: " << GIT_COMMIT_ID << std::endl;
    std::cout << "Build time: " << BUILD_TIME << std::endl;

    Test test;
    test.a = 1;
    test.b = 2.0;
    test.c = "3";
    std::cout << test.a << " " << test.b << " " << test.c << std::endl;

    auto thread = std::thread([&test]() {
        std::cout << "thread id: " << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));
        test.a = 4;
        test.b = 5.0;
        test.c = "6";
        std::cout << test.a << " " << test.b << " " << test.c << std::endl;
    });
    thread.detach();

    std::cout << "main thread id: " << std::this_thread::get_id() << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    // pthread_exit(0);
    return 0;
}
