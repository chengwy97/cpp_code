#include <filesystem>
#include <fstream>
#include <gsl/assert>
#include <gsl/gsl>
#include <iostream>
#include <string>

#include "filesystem_wrapper/filesystem_wrapper.hpp"
#include "logging/log_def.hpp"

namespace test {
class Test {
   public:
    Test() { std::cout << "Test constructor" << std::endl; }
    ~Test() { std::cout << "Test destructor" << std::endl; }
    int a = 10;
};

void print_test(const Test& test) {
    std::cout << "Test print: " << test.a << std::endl;
}
}  // namespace test

void test_gsl_finally() {
    auto finally_action = gsl::finally([]() { std::cout << "finally action" << std::endl; });

    std::cout << "finally action" << std::endl;

    std::cout << 1 << std::endl;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    std::string_view str      = "test";
    auto             str_span = gsl::span<const char>(str.data(), str.size());
    for (auto& c : str_span) {
        std::cout << c;
    }
    std::cout << std::endl;

    std::vector<std::string> vec      = {"test", "test2", "test3"};
    auto                     vec_span = gsl::span<std::string>(vec);
    for (auto& s : vec_span) {
        std::cout << s << std::endl;
    }

    Expects(1 > 0);
    Ensures(1 > 0);

    test_gsl_finally();

    gsl::owner<int*> owner_int = new int(1);
    std::cout << *owner_int << std::endl;

    // adl 规则
    test::Test test;
    print_test(test);
    return 0;
}
