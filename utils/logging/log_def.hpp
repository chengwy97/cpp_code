#pragma once

#include "logging.hpp"

#define UTILS_LOG_INIT(log_name, log_level) \
    utils::logging::spdlog_log::init_logging(log_name, log_level);

#define UTILS_LOG_FLUSH() utils::logging::spdlog_log::flush_logging();

#define UTILS_LOG_TRACE(fmt, ...) UTILS_SPDLOG_LOG_TRACE(fmt, ##__VA_ARGS__)
#define UTILS_LOG_DEBUG(fmt, ...) UTILS_SPDLOG_LOG_DEBUG(fmt, ##__VA_ARGS__)
#define UTILS_LOG_INFO(fmt, ...) UTILS_SPDLOG_LOG_INFO(fmt, ##__VA_ARGS__)
#define UTILS_LOG_WARN(fmt, ...) UTILS_SPDLOG_LOG_WARN(fmt, ##__VA_ARGS__)
#define UTILS_LOG_ERROR(fmt, ...) UTILS_SPDLOG_LOG_ERROR(fmt, ##__VA_ARGS__)
#define UTILS_LOG_FATAL(fmt, ...) UTILS_SPDLOG_LOG_FATAL(fmt, ##__VA_ARGS__)
