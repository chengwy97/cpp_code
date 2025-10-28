#include <iostream>

#include "logging/log_def.hpp"

int main() {
    int ret = UTILS_LOG_INIT("/tmp/test.log", "debug");
    if (ret != 0) {
        std::cerr << "Failed to initialize logging" << std::endl;
        return 1;
    }

    UTILS_LOG_DEBUG("This is a debug message");
    UTILS_LOG_TRACE("This is a trace message: {}", 123);
    UTILS_LOG_DEBUG("This is a debug message: {}", 456);
    UTILS_LOG_INFO("This is a info message: {}", 789);
    UTILS_LOG_WARN("This is a warn message: {}", 101);
    UTILS_LOG_ERROR("This is a error message: {}", 112);
    UTILS_LOG_FATAL("This is a fatal message: {}", 113);

    // flush async sink
    UTILS_LOG_FLUSH();

    return 0;
}
