#include "logging.hpp"

#include <algorithm>
#include <cctype>
#include <memory>
#include <vector>

namespace utils {
namespace logging {
namespace spdlog_log {

std::shared_ptr<spdlog::logger> g_logger = nullptr;

int init_logging(const std::string& log_name, const std::string& log_level) {
    try {
        // 创建 sinks
        std::vector<spdlog::sink_ptr> sinks;

        // 文件 sink - 轮转文件
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_name, 10485760, 10);  // 10MB, 10 files

        // 控制台 sink - 彩色输出
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

        sinks.push_back(file_sink);
        sinks.push_back(console_sink);

        // 创建 logger
        g_logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());

        // 设置默认 logger
        spdlog::set_default_logger(g_logger);

        // 解析日志等级字符串
        std::string lower_level = log_level;
        std::transform(lower_level.begin(), lower_level.end(), lower_level.begin(), ::tolower);

        spdlog::level::level_enum level = spdlog::level::info;
        if (lower_level == "trace")
            level = spdlog::level::trace;
        else if (lower_level == "debug")
            level = spdlog::level::debug;
        else if (lower_level == "info")
            level = spdlog::level::info;
        else if (lower_level == "warning")
            level = spdlog::level::warn;
        else if (lower_level == "error")
            level = spdlog::level::err;
        else if (lower_level == "fatal")
            level = spdlog::level::critical;

        // 设置日志级别
        spdlog::set_level(level);

        // 设置日志格式: [时间] [级别] [线程ID] 消息
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] [%g:%#] %v");

        return 0;
    } catch (const spdlog::spdlog_ex& ex) {
        return -1;
    }
}

void flush_logging() {
    if (g_logger) {
        g_logger->flush_on(spdlog::level::critical);
    }
    spdlog::get("multi_sink")->flush();
}

std::shared_ptr<spdlog::logger> get_default_logger() {
    return g_logger;
}

}  // namespace spdlog_log
}  // namespace logging
}  // namespace utils
