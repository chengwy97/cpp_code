#pragma once

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

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

}  // namespace spdlog_log
}  // namespace logging
}  // namespace utils

#define UTILS_SPDLOG_LOG_TRACE(fmt, ...) SPDLOG_TRACE(fmt, ##__VA_ARGS__)
#define UTILS_SPDLOG_LOG_DEBUG(fmt, ...) SPDLOG_DEBUG(fmt, ##__VA_ARGS__)
#define UTILS_SPDLOG_LOG_INFO(fmt, ...) SPDLOG_INFO(fmt, ##__VA_ARGS__)
#define UTILS_SPDLOG_LOG_WARN(fmt, ...) SPDLOG_WARN(fmt, ##__VA_ARGS__)
#define UTILS_SPDLOG_LOG_ERROR(fmt, ...) SPDLOG_ERROR(fmt, ##__VA_ARGS__)
#define UTILS_SPDLOG_LOG_FATAL(fmt, ...) SPDLOG_CRITICAL(fmt, ##__VA_ARGS__)
