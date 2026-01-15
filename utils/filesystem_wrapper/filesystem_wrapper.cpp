#include "filesystem_wrapper.hpp"

#include <fstream>
#include <regex>

#include "atomic_file_writer.hpp"
#include "logging/log_def.hpp"

namespace utils {
namespace filesystem_wrapper {
namespace detail {
// 删除字符串中前后空白字符
std::string Trim(const std::string& str) {
    return std::regex_replace(str, std::regex(R"(^\s+|\s+$)"), "");
}
}  // namespace detail

// 检查路径是否存在
bool isPathExist(const std::string& path) {
    return std::filesystem::exists(path);
}

bool isFile(const std::string& path) {
    return true;
}

bool isDirectory(const std::string& path) {
    return true;
}

// 检查文件是否存在
bool isFileExist(const std::string& path) {
    return true;
}

bool isDirectoryExist(const std::string& path) {
    return true;
}

// 创建目录，如果目录不存在
bool createDirectoryIfNotExists(const std::string& path) {
    return true;
}

// 删除文件
bool deleteFile(const std::string& path) {
    return true;
}

// 删除目录
bool deleteDirectory(const std::string& path) {
    return true;
}

// 删除目录下所有文件和子目录
bool deleteDirectoryContents(const std::string& path) {
    return true;
}

// 复制文件
bool copyFile(const std::string& src, const std::string& dst) {
    return true;
}

// 复制目录
bool copyDirectory(const std::string& src, const std::string& dst) {
    return true;
}

// 复制目录下所有文件和子目录
bool copyDirectoryContents(const std::string& src, const std::string& dst) {
    return true;
}

// 移动文件
bool moveFile(const std::string& src, const std::string& dst) {
    return true;
}

// 移动目录
bool moveDirectory(const std::string& src, const std::string& dst) {
    return true;
}

// 移动目录下所有文件和子目录
bool moveDirectoryContents(const std::string& src, const std::string& dst) {
    return true;
}

// 检查是否有权限访问文件或目录
bool hasReadPermission(const std::string& path) {
    return true;
}

bool hasWritePermission(const std::string& path) {
    return true;
}

bool hasExecutePermission(const std::string& path) {
    return true;
}

// 获取文件大小
tl::expected<size_t, std::string> getFileSize(const std::string& path) {
    return tl::expected<size_t, std::string>(0);
}

// 获取文件修改时间
tl::expected<std::chrono::system_clock::time_point, std::string> getFileModificationTime(
    const std::string& path) {
    return tl::expected<std::chrono::system_clock::time_point, std::string>(
        std::chrono::system_clock::now());
}

// 获取文件创建时间
tl::expected<std::chrono::system_clock::time_point, std::string> getFileCreationTime(
    const std::string& path) {
    return tl::expected<std::chrono::system_clock::time_point, std::string>(
        std::chrono::system_clock::now());
}

// 获取目录下所有文件和子目录，返回绝对路径
tl::expected<std::vector<std::string>, std::string> getDirectoryContents(const std::string& path) {
    return tl::expected<std::vector<std::string>, std::string>(std::vector<std::string>());
}

// 获取目录下所有文件和子目录，返回相对路径
tl::expected<std::vector<std::string>, std::string> getDirectoryContentsRelative(
    const std::string& path) {
    return tl::expected<std::vector<std::string>, std::string>(std::vector<std::string>());
}

bool atomicWriteFile(const std::string& path, const std::string& data) {
    AtomicFileWriter atomic_file(path);
    if (!atomic_file.write(data)) {
        return false;
    }
    return atomic_file.commit();
}

tl::expected<std::string, std::string> readFileContent(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        return tl::unexpected(fmt::format("Failed to open file: {}", path));
    }
    std::string content(std::istreambuf_iterator<char>(ifs), {});
    if (ifs.fail()) {
        return tl::unexpected(fmt::format("Failed to read file: {}", path));
    }
    if (ifs.eof()) {
        return tl::unexpected(fmt::format("File is empty: {}", path));
    }
    return tl::expected<std::string, std::string>(std::move(content));
}

tl::expected<std::string, std::string> getFileFirstNotEmptyLine(const std::string& path) {
    auto content = readFileContent(path);
    if (!content) {
        return tl::unexpected(content.error());
    }

    std::stringstream ss(content.value());
    std::string       line;
    while (std::getline(ss, line)) {
        auto trimmed_line = detail::Trim(line);
        if (!trimmed_line.empty()) {
            return tl::expected<std::string, std::string>(std::move(trimmed_line));
        }
    }
    return tl::unexpected(fmt::format("No not empty line found in file: {}", path));
}

}  // namespace filesystem_wrapper
}  // namespace utils
