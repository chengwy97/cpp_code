#pragma once

#include <chrono>
#include <string>
#include <tl/tl_expected.hpp>
#include <vector>

namespace utils {
namespace filesystem_wrapper {

// 检查路径是否存在
bool isPathExist(const std::string& path);
bool isFile(const std::string& path);
bool isDirectory(const std::string& path);
// 检查文件是否存在
bool isFileExist(const std::string& path);
bool isDirectoryExist(const std::string& path);

// 创建目录，如果目录不存在
bool createDirectoryIfNotExists(const std::string& path);

// 删除文件
bool deleteFile(const std::string& path);
// 删除目录
bool deleteDirectory(const std::string& path);
// 删除目录下所有文件和子目录
bool deleteDirectoryContents(const std::string& path);

// 复制文件
bool copyFile(const std::string& src, const std::string& dst);
// 复制目录
bool copyDirectory(const std::string& src, const std::string& dst);
// 复制目录下所有文件和子目录
bool copyDirectoryContents(const std::string& src, const std::string& dst);

// 移动文件
bool moveFile(const std::string& src, const std::string& dst);
// 移动目录
bool moveDirectory(const std::string& src, const std::string& dst);
// 移动目录下所有文件和子目录
bool moveDirectoryContents(const std::string& src, const std::string& dst);

// 检查是否有权限访问文件或目录
bool hasReadPermission(const std::string& path);
bool hasWritePermission(const std::string& path);
bool hasExecutePermission(const std::string& path);

// 获取文件大小
tl::expected<size_t, std::string> getFileSize(const std::string& path);
// 获取文件修改时间
tl::expected<std::chrono::system_clock::time_point, std::string> getFileModificationTime(
    const std::string& path);
// 获取文件创建时间
tl::expected<std::chrono::system_clock::time_point, std::string> getFileCreationTime(
    const std::string& path);

// 获取目录下所有文件和子目录，返回绝对路径
tl::expected<std::vector<std::string>, std::string> getDirectoryContents(const std::string& path);
// 获取目录下所有文件和子目录，返回相对路径
tl::expected<std::vector<std::string>, std::string> getDirectoryContentsRelative(
    const std::string& path);

// 原子写文件
bool atomicWriteFile(const std::string& path, const std::string& data);
tl::expected<std::string, std::string> readFileContent(const std::string& path);
tl::expected<std::string, std::string> getFileFirstNotEmptyLine(const std::string& path);

}  // namespace filesystem_wrapper
}  // namespace utils
