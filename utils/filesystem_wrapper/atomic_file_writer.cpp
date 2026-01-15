#include "atomic_file_writer.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

#include "logging/log_def.hpp"

namespace utils {
namespace filesystem_wrapper {

AtomicFileWriter::AtomicFileWriter(const std::filesystem::path& path)
    : path_(path), file_(nullptr) {
    // 确保父目录存在
    if (!path_.parent_path().empty() && !std::filesystem::exists(path_.parent_path())) {
        std::filesystem::create_directories(path_.parent_path());
    }

    // 生成临时文件路径
    temp_path_ = fmt::format("{}.tmp", (path_.parent_path() / path_.filename()).string());

    // 打开临时文件，如果失败 file_ 为 nullptr
    file_ = fopen(temp_path_.c_str(), "wb");
    if (!file_) {
        // 文件打开失败，但构造函数不能抛出异常
        // 后续的 write() 和 commit() 会检查 file_ 是否为 nullptr
        UTILS_LOG_ERROR("Failed to open file: {}", temp_path_.string());
        return;
    }
}

AtomicFileWriter::~AtomicFileWriter() {
    // 如果文件仍然打开，关闭它
    if (file_) {
        fclose(file_);
        file_ = nullptr;
    }

    // 如果临时文件存在（说明 commit() 未成功），删除它
    if (std::filesystem::exists(temp_path_)) {
        UTILS_LOG_ERROR("File not committed or committed failed: {}", temp_path_.string());
        std::filesystem::remove(temp_path_);
    }
}

bool AtomicFileWriter::write(const std::string& data) {
    if (!file_) {
        UTILS_LOG_ERROR("File not opened: {}", temp_path_.string());
        return false;
    }

    size_t written = fwrite(data.c_str(), 1, data.size(), file_);
    if (written != data.size()) {
        UTILS_LOG_ERROR("Failed to write data to file: {}", temp_path_.string());
        return false;
    }
    return true;
}

bool AtomicFileWriter::commit() {
    if (!file_) {
        UTILS_LOG_ERROR("File not opened: {}", temp_path_.string());
        return false;
    }

    // 刷新并同步文件数据到磁盘
    if (fflush(file_) != 0) {
        UTILS_LOG_ERROR("Failed to flush file: {}", temp_path_.string());
        return false;
    }

    int fd = fileno(file_);
    if (fd == -1) {
        UTILS_LOG_ERROR("Failed to get file descriptor: {}", temp_path_.string());
        return false;
    }

    if (fsync(fd) != 0) {
        UTILS_LOG_ERROR("Failed to sync file: {}", temp_path_.string());
        return false;
    }

    fclose(file_);
    file_ = nullptr;

    // 在重命名之前，先检查目录是否可访问
    // 使用 O_RDONLY 而不是 O_WRONLY，因为只需要读取目录元数据
    std::filesystem::path parent_dir = path_.parent_path();
    if (parent_dir.empty()) {
        parent_dir = ".";
    }

    int dir_fd = open(parent_dir.c_str(), O_RDONLY | O_DIRECTORY);
    if (dir_fd == -1) {
        // 无法打开目录，但文件已经同步，仍然尝试重命名
        // 如果重命名失败，临时文件会在析构时被删除
        UTILS_LOG_WARN("Failed to open directory: {}", parent_dir.string());
    }

    // 执行原子重命名操作
    std::error_code ec;
    std::filesystem::rename(temp_path_, path_, ec);
    if (ec) {
        if (dir_fd != -1) {
            close(dir_fd);
        }
        UTILS_LOG_ERROR("Failed to rename file: {}", temp_path_.string());
        return false;
    }

    // 同步目录元数据，确保重命名操作持久化
    if (dir_fd != -1) {
        if (fsync(dir_fd) != 0) {
            UTILS_LOG_ERROR("Failed to sync directory: {}", parent_dir.string());
        }
        if (close(dir_fd) != 0) {
            UTILS_LOG_ERROR("Failed to close directory: {}", parent_dir.string());
        }
    }

    return true;
}

}  // namespace filesystem_wrapper
}  // namespace utils
