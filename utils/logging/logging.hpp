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

#define UTILS_SPDLOG_LOG_TRACE(fmt, ...)                                         \
    do {                                                                         \
        if (auto logger = ::utils::logging::spdlog_log::get_default_logger()) {  \
            logger->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, \
                        spdlog::level::trace, fmt, ##__VA_ARGS__);               \
        }                                                                        \
    } while (0)

#define UTILS_SPDLOG_LOG_DEBUG(fmt, ...)                                         \
    do {                                                                         \
        if (auto logger = ::utils::logging::spdlog_log::get_default_logger()) {  \
            logger->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, \
                        spdlog::level::debug, fmt, ##__VA_ARGS__);               \
        }                                                                        \
    } while (0)

#define UTILS_SPDLOG_LOG_INFO(fmt, ...)                                          \
    do {                                                                         \
        if (auto logger = ::utils::logging::spdlog_log::get_default_logger()) {  \
            logger->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, \
                        spdlog::level::info, fmt, ##__VA_ARGS__);                \
        }                                                                        \
    } while (0)

#define UTILS_SPDLOG_LOG_WARN(fmt, ...)                                          \
    do {                                                                         \
        if (auto logger = ::utils::logging::spdlog_log::get_default_logger()) {  \
            logger->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, \
                        spdlog::level::warn, fmt, ##__VA_ARGS__);                \
        }                                                                        \
    } while (0)

#define UTILS_SPDLOG_LOG_ERROR(fmt, ...)                                         \
    do {                                                                         \
        if (auto logger = ::utils::logging::spdlog_log::get_default_logger()) {  \
            logger->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, \
                        spdlog::level::err, fmt, ##__VA_ARGS__);                 \
        }                                                                        \
    } while (0)

#define UTILS_SPDLOG_LOG_FATAL(fmt, ...)                                         \
    do {                                                                         \
        if (auto logger = ::utils::logging::spdlog_log::get_default_logger()) {  \
            logger->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, \
                        spdlog::level::critical, fmt, ##__VA_ARGS__);            \
        }                                                                        \
    } while (0)
