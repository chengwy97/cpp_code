#pragma once

#include <boost/log/core.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <format>
#include <source_location>
#include <string>

namespace utils {
namespace logging {
namespace boost_log {
int  init_logging(const std::string& log_name, const std::string& log_level);
void flush_logging();
}  // namespace boost_log
}  // namespace logging
}  // namespace utils

#define UTILS_BOOST_LOG_TRACE(fmt, ...)                                                            \
    {                                                                                              \
        auto location = std::source_location::current();                                           \
        BOOST_LOG_TRIVIAL(trace) << std::format("[{}:{}] ", location.file_name(), location.line()) \
                                 << std::format(fmt, ##__VA_ARGS__);                               \
    }
#define UTILS_BOOST_LOG_DEBUG(fmt, ...)                                                            \
    {                                                                                              \
        auto location = std::source_location::current();                                           \
        BOOST_LOG_TRIVIAL(debug) << std::format("[{}:{}] ", location.file_name(), location.line()) \
                                 << std::format(fmt, ##__VA_ARGS__);                               \
    }
#define UTILS_BOOST_LOG_INFO(fmt, ...)                                                            \
    {                                                                                             \
        auto location = std::source_location::current();                                          \
        BOOST_LOG_TRIVIAL(info) << std::format("[{}:{}] ", location.file_name(), location.line()) \
                                << std::format(fmt, ##__VA_ARGS__);                               \
    }
#define UTILS_BOOST_LOG_WARN(fmt, ...)                                        \
    {                                                                         \
        auto location = std::source_location::current();                      \
        BOOST_LOG_TRIVIAL(warning)                                            \
            << std::format("[{}:{}] ", location.file_name(), location.line()) \
            << std::format(fmt, ##__VA_ARGS__);                               \
    }
#define UTILS_BOOST_LOG_ERROR(fmt, ...)                                                            \
    {                                                                                              \
        auto location = std::source_location::current();                                           \
        BOOST_LOG_TRIVIAL(error) << std::format("[{}:{}] ", location.file_name(), location.line()) \
                                 << std::format(fmt, ##__VA_ARGS__);                               \
    }
#define UTILS_BOOST_LOG_FATAL(fmt, ...)                                                            \
    {                                                                                              \
        auto location = std::source_location::current();                                           \
        BOOST_LOG_TRIVIAL(fatal) << std::format("[{}:{}] ", location.file_name(), location.line()) \
                                 << std::format(fmt, ##__VA_ARGS__);                               \
    }
