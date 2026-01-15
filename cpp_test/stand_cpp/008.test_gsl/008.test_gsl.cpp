#include <filesystem>
#include <fstream>
#include <gsl/assert>
#include <gsl/gsl>
#include <iostream>
#include <string>

#include "filesystem_wrapper/filesystem_wrapper.hpp"
#include "logging/log_def.hpp"

int main(int argc, char* argv[]) {
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
    return 0;
}
