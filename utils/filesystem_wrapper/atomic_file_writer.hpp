#pragma once

#include <cstdio>
#include <filesystem>
#include <string>

namespace utils {
namespace filesystem_wrapper {

class AtomicFileWriter {
   public:
    explicit AtomicFileWriter(const std::filesystem::path& path);
    ~AtomicFileWriter();

    bool write(const std::string& data);
    bool commit();

   private:
    std::filesystem::path path_;
    std::filesystem::path temp_path_;

    FILE* file_;
};
}  // namespace filesystem_wrapper
}  // namespace utils
