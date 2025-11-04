#pragma once

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <source_location>
#include <sstream>
#include <string>

namespace utils {
namespace logging {
namespace spdlog_log {

// 初始化日志系统
int init_logging(const std::string& log_name, const std::string& log_level);

// 刷新日志
void flush_logging();

// 获取默认日志记录器
std::shared_ptr<spdlog::logger> get_default_logger();

// 流式日志包装类
class LogStream {
   public:
    LogStream(spdlog::level::level_enum   level,
              const std::source_location& loc = std::source_location::current())
        : level_(level), loc_(loc) {}

    ~LogStream() {
        if (logger_) {
            auto msg = stream_.str();
            if (!msg.empty()) {
                spdlog::source_loc spdlog_loc{loc_.file_name(), static_cast<int>(loc_.line()),
                                              loc_.function_name()};
                logger_->log(spdlog_loc, level_, "{}", msg);
            }
        }
    }

    // 禁止拷贝和移动
    LogStream(const LogStream&)            = delete;
    LogStream& operator=(const LogStream&) = delete;
    LogStream(LogStream&&)                 = delete;
    LogStream& operator=(LogStream&&)      = delete;

    // 重载 << 操作符
    template <typename T>
    LogStream& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }

    // 支持 std::endl 等流操作符
    LogStream& operator<<(std::ostream& (*manip)(std::ostream&)) {
        manip(stream_);
        return *this;
    }

   private:
    spdlog::level::level_enum       level_;
    std::source_location            loc_;
    std::ostringstream              stream_;
    std::shared_ptr<spdlog::logger> logger_{get_default_logger()};
};

}  // namespace spdlog_log
}  // namespace logging
}  // namespace utils

#define UTILS_SPDLOG_LOG_TRACE(fmt, ...) SPDLOG_TRACE(fmt, ##__VA_ARGS__)
#define UTILS_SPDLOG_LOG_DEBUG(fmt, ...) SPDLOG_DEBUG(fmt, ##__VA_ARGS__)
#define UTILS_SPDLOG_LOG_INFO(fmt, ...) SPDLOG_INFO(fmt, ##__VA_ARGS__)
#define UTILS_SPDLOG_LOG_WARN(fmt, ...) SPDLOG_WARN(fmt, ##__VA_ARGS__)
#define UTILS_SPDLOG_LOG_ERROR(fmt, ...) SPDLOG_ERROR(fmt, ##__VA_ARGS__)
#define UTILS_SPDLOG_LOG_FATAL(fmt, ...) SPDLOG_CRITICAL(fmt, ##__VA_ARGS__)
