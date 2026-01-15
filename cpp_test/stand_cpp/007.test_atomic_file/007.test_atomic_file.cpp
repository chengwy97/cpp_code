#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "filesystem_wrapper/filesystem_wrapper.hpp"
#include "logging/log_def.hpp"

int main(int argc, char* argv[]) {
    UTILS_LOG_INIT("/tmp/test_atomic_file.log", "info");

    std::string file_path = "/tmp/test_atomic_file.txt";
    auto        result    = utils::filesystem_wrapper::atomicWriteFile(file_path, "test");
    if (!result) {
        UTILS_LOG_ERROR("Failed to write file: {}", file_path);
        return 1;
    }

    auto content = utils::filesystem_wrapper::readFileContent(file_path);
    if (!content) {
        UTILS_LOG_ERROR("Failed to read file: {}", content.error());
        return 1;
    }
    UTILS_LOG_INFO("content: {}", content.value());

    result = utils::filesystem_wrapper::atomicWriteFile(file_path, "test2");
    if (!result) {
        UTILS_LOG_ERROR("Failed to write file: {}", file_path);
        return 1;
    }
    content = utils::filesystem_wrapper::readFileContent(file_path);
    if (!content) {
        UTILS_LOG_ERROR("Failed to read file: {}", content.error());
        return 1;
    }
    UTILS_LOG_INFO("content: {}", content.value());

    std::string file_content = R"(

       line1
       line2
       line3
    )";
    result                   = utils::filesystem_wrapper::atomicWriteFile(file_path, file_content);
    if (!result) {
        UTILS_LOG_ERROR("Failed to write file: {}", file_path);
        return 1;
    }
    content = utils::filesystem_wrapper::readFileContent(file_path);
    if (!content) {
        UTILS_LOG_ERROR("Failed to read file: {}", content.error());
        return 1;
    }
    UTILS_LOG_INFO("content: {}", content.value());

    auto first_line = utils::filesystem_wrapper::getFileFirstNotEmptyLine(file_path);
    if (!first_line) {
        UTILS_LOG_ERROR("Failed to get first not empty line: {}", first_line.error());
        return 1;
    }
    UTILS_LOG_INFO("first line: {}", first_line.value());

    file_content = R"(
       xxxxxxx line1 xxxxxxx
       xxxxxxx line2 xxxxxxx
       xxxxxxx line3 xxxxxxx
    )";
    result       = utils::filesystem_wrapper::atomicWriteFile(file_path, file_content);
    if (!result) {
        UTILS_LOG_ERROR("Failed to write file: {}", file_path);
        return 1;
    }

    content = utils::filesystem_wrapper::readFileContent(file_path);
    if (!content) {
        UTILS_LOG_ERROR("Failed to read file: {}", content.error());
        return 1;
    }
    UTILS_LOG_INFO("content: {}", content.value());

    first_line = utils::filesystem_wrapper::getFileFirstNotEmptyLine(file_path);
    if (!first_line) {
        UTILS_LOG_ERROR("Failed to get first not empty line: {}", first_line.error());
        return 1;
    }
    UTILS_LOG_INFO("first line: {}", first_line.value());

    UTILS_LOG_FLUSH();
    return 0;
}
