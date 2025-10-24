#include "logging.hpp"

#include <algorithm>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/attributes/current_thread_id.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <cctype>

namespace utils {
namespace logging {
namespace boost_log {

int init_logging(const std::string& log_name, const std::string& log_level) {
    namespace logging  = boost::log;
    namespace expr     = boost::log::expressions;
    namespace sinks    = boost::log::sinks;
    namespace keywords = boost::log::keywords;

    // 创建异步文件 sink
    using file_sink_t = sinks::asynchronous_sink<sinks::text_file_backend>;
    auto sink = boost::make_shared<file_sink_t>(boost::make_shared<sinks::text_file_backend>(
        keywords::file_name     = log_name,
        keywords::rotation_size = 10 * 1024 * 1024,  // 10MB
        keywords::open_mode     = std::ios::app,
        keywords::time_based_rotation =
            sinks::file::rotation_at_time_point(0, 0, 0),  // 每天0点轮转
        keywords::max_files = 10));

    // 设置日志格式: [线程ID] 时间戳 等级 消息
    sink->set_formatter(
        expr::stream
        << "[" << expr::attr<boost::log::attributes::current_thread_id::value_type>("ThreadID")
        << "] " << "["
        << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
        << "] " << " [" << expr::attr<logging::trivial::severity_level>("Severity") << "] " << "["
        << expr::attr<std::string>("File") << ":" << expr::attr<int>("Line") << "] "
        << expr::smessage);

    logging::core::get()->add_sink(sink);

    // 添加基础属性（TimeStamp、ThreadID等）
    logging::add_common_attributes();

    // 解析日志等级字符串
    std::string lower_level = log_level;
    std::transform(lower_level.begin(), lower_level.end(), lower_level.begin(), ::tolower);

    logging::trivial::severity_level level = logging::trivial::info;
    if (lower_level == "trace")
        level = logging::trivial::trace;
    else if (lower_level == "debug")
        level = logging::trivial::debug;
    else if (lower_level == "info")
        level = logging::trivial::info;
    else if (lower_level == "warning")
        level = logging::trivial::warning;
    else if (lower_level == "error")
        level = logging::trivial::error;
    else if (lower_level == "fatal")
        level = logging::trivial::fatal;

    // ✅ 设置过滤器（核心）
    logging::core::get()->set_filter(expr::attr<logging::trivial::severity_level>("Severity") >=
                                     level);

    return 0;
}

void flush_logging() {
    boost::log::core::get()->flush();
}

}  // namespace boost_log
}  // namespace logging
}  // namespace utils
