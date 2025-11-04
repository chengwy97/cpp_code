#pragma once

#include "logging.hpp"

#define UTILS_LOG_INIT(log_name, log_level) \
    utils::logging::spdlog_log::init_logging(log_name, log_level);

#define UTILS_LOG_FLUSH() utils::logging::spdlog_log::flush_logging();

// 格式化字符串风格的日志宏（保留兼容性）
// 用法: UTILS_LOG_TRACE("message {}", value)
#define UTILS_LOG_TRACE(fmt, ...) UTILS_SPDLOG_LOG_TRACE(fmt, ##__VA_ARGS__)
#define UTILS_LOG_DEBUG(fmt, ...) UTILS_SPDLOG_LOG_DEBUG(fmt, ##__VA_ARGS__)
#define UTILS_LOG_INFO(fmt, ...) UTILS_SPDLOG_LOG_INFO(fmt, ##__VA_ARGS__)
#define UTILS_LOG_WARN(fmt, ...) UTILS_SPDLOG_LOG_WARN(fmt, ##__VA_ARGS__)
#define UTILS_LOG_ERROR(fmt, ...) UTILS_SPDLOG_LOG_ERROR(fmt, ##__VA_ARGS__)
#define UTILS_LOG_FATAL(fmt, ...) UTILS_SPDLOG_LOG_FATAL(fmt, ##__VA_ARGS__)

// 流式日志宏 - 返回临时 LogStream 对象，支持 << 操作符
// 用法: UTILS_LOG_STREAM_TRACE() << "message" << value
// 或使用统一宏: UTILS_LOG(TRACE) << "message" << value
// 使用 std::source_location 自动捕获文件路径、行号和函数名信息（C++20）
#define UTILS_LOG_STREAM_TRACE() \
    utils::logging::spdlog_log::LogStream(spdlog::level::trace, std::source_location::current())
#define UTILS_LOG_STREAM_DEBUG() \
    utils::logging::spdlog_log::LogStream(spdlog::level::debug, std::source_location::current())
#define UTILS_LOG_STREAM_INFO() \
    utils::logging::spdlog_log::LogStream(spdlog::level::info, std::source_location::current())
#define UTILS_LOG_STREAM_WARN() \
    utils::logging::spdlog_log::LogStream(spdlog::level::warn, std::source_location::current())
#define UTILS_LOG_STREAM_ERROR() \
    utils::logging::spdlog_log::LogStream(spdlog::level::err, std::source_location::current())
#define UTILS_LOG_STREAM_FATAL() \
    utils::logging::spdlog_log::LogStream(spdlog::level::critical, std::source_location::current())

// 统一流式日志宏 - UTILS_LOG(TRACE) << "message" << value
// 注意: level 必须是 TRACE, DEBUG, INFO, WARN, ERROR, FATAL 之一（大写）
#define UTILS_LOG(level) UTILS_LOG_STREAM_##level()
