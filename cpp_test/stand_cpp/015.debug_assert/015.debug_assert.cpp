// ============================================================================
// debug_assert 库使用教学
//
// debug_assert 是 foonathan 开发的轻量级断言库，相比标准 assert() 提供：
//   - 模块化：每个模块可以有独立的断言级别和处理器
//   - 分级断言：通过 level<N> 控制断言的严格程度
//   - 自定义处理：可以自定义断言失败时的行为（打印、抛异常等）
//   - 零开销：未启用的断言级别在编译期被完全优化掉
//
// 核心概念：
//   Handler  —— 一个结构体，继承 set_level<N> 和一个处理基类
//   Level    —— 断言级别，level<1> 最基础，level<N> 越高越严格
//   当 Handler::level >= N 时，level<N> 的断言才会生效
// ============================================================================

#include <cstdio>
#include <debug_assert.hpp>
#include <format>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

// ============================================================================
// 第一部分：最简单的用法 —— 使用 default_handler
// ============================================================================

// 定义一个模块的断言处理器：
//   set_level<1>      表示只启用 level<=1 的断言
//   default_handler    断言失败时打印到 stderr 然后 abort
struct module_a_assert : debug_assert::set_level<1>, debug_assert::default_handler {};

void demo_basic() {
    std::cout << "=== 第一部分：基础用法 ===" << std::endl;

    int x = 42;

    // 基本断言：表达式为 true 时通过，false 时触发 handler
    DEBUG_ASSERT(x == 42, module_a_assert{});
    std::cout << "  DEBUG_ASSERT(x == 42) passed" << std::endl;

    // 带消息的断言
    DEBUG_ASSERT(x > 0, module_a_assert{}, "x must be positive");
    std::cout << "  DEBUG_ASSERT(x > 0, msg) passed" << std::endl;

    // 带 level 的断言：level<1> 会生效（因为 handler level=1）
    DEBUG_ASSERT(x != 0, module_a_assert{}, debug_assert::level<1>{}, "x must not be zero");
    std::cout << "  DEBUG_ASSERT(level<1>) passed" << std::endl;

    // level<2> 的断言不会生效（因为 handler level=1 < 2），直接被优化掉
    DEBUG_ASSERT(x == 999, module_a_assert{}, debug_assert::level<2>{}, "this won't fire");
    std::cout << "  DEBUG_ASSERT(level<2>) skipped (handler level=1 < 2)" << std::endl;

    std::cout << std::endl;
}

// ============================================================================
// 第二部分：分级断言 —— 不同严格程度
// ============================================================================

// 高级别的 handler：启用 level 1~3 的所有断言
struct strict_assert : debug_assert::set_level<3>, debug_assert::default_handler {};

// 低级别的 handler：只启用 level 1
struct relaxed_assert : debug_assert::set_level<1>, debug_assert::default_handler {};

void demo_levels() {
    std::cout << "=== 第二部分：分级断言 ===" << std::endl;

    int value = 50;

    // strict_assert (level=3)：level<1>, level<2>, level<3> 都会检查
    DEBUG_ASSERT(value > 0, strict_assert{}, debug_assert::level<1>{}, "L1: positive");
    DEBUG_ASSERT(value < 100, strict_assert{}, debug_assert::level<2>{}, "L2: < 100");
    DEBUG_ASSERT(value == 50, strict_assert{}, debug_assert::level<3>{}, "L3: exactly 50");
    std::cout << "  strict_assert: level<1>, <2>, <3> all checked" << std::endl;

    // relaxed_assert (level=1)：只有 level<1> 会检查，其余被编译期忽略
    DEBUG_ASSERT(value > 0, relaxed_assert{}, debug_assert::level<1>{}, "L1: positive");
    DEBUG_ASSERT(value == 999, relaxed_assert{}, debug_assert::level<2>{}, "L2: skipped");
    DEBUG_ASSERT(value == 999, relaxed_assert{}, debug_assert::level<3>{}, "L3: skipped");
    std::cout << "  relaxed_assert: only level<1> checked, <2>/<3> optimized away" << std::endl;

    std::cout << std::endl;
}

// ============================================================================
// 第三部分：自定义 handler —— 抛出异常
// ============================================================================

// 自定义 handler：断言失败时抛出 runtime_error 而不是 abort
// 需要继承 allow_exception 来告知库"这个 handler 会抛异常"
struct throwing_assert : debug_assert::set_level<1>, debug_assert::allow_exception {
    static void handle(const debug_assert::source_location& loc, const char* expression,
                       const char* message = nullptr) {
        std::string msg = std::string("[assert failed] ") + loc.file_name + ":" +
                          std::to_string(loc.line_number) + ": '" + expression + "'";
        if (message) {
            msg += " - ";
            msg += message;
        }
        throw std::runtime_error(msg);
    }
};

void demo_throwing_handler() {
    std::cout << "=== 第三部分：抛异常的 handler ===" << std::endl;

    try {
        DEBUG_ASSERT(1 == 2, throwing_assert{}, "one is not two");
        std::cout << "  This line should not be reached" << std::endl;
    } catch (const std::runtime_error& e) {
        std::cout << "  Caught exception: " << e.what() << std::endl;
    }

    std::cout << std::endl;
}

// ============================================================================
// 第四部分：自定义 handler —— 带额外参数
// ============================================================================

// handler 的 handle() 可以接受任意额外参数
struct verbose_assert : debug_assert::set_level<2>, debug_assert::default_handler {
    // 除了默认的 (loc, expr) 外，还接受 (loc, expr, message, error_code)
    static void handle(const debug_assert::source_location& loc, const char* expression,
                       const char* message, int error_code) noexcept {
        std::fprintf(stderr, "[verbose] %s:%u: '%s' failed - %s (code: %d)\n", loc.file_name,
                     loc.line_number, expression, message, error_code);
    }
};

void demo_extra_args() {
    std::cout << "=== 第四部分：带额外参数的 handler ===" << std::endl;

    int error_code = -42;
    // 将额外参数传给 handler
    DEBUG_ASSERT(error_code <= 0, verbose_assert{}, "invalid error code", error_code);
    // ↑ 这里会触发断言失败，调用 handle(loc, "error_code >= 0", "invalid error code", -42)
    // 但因为 default_handler 后面跟了 abort()，所以不会执行到这里
    // 为了演示，我们用不会失败的版本
    DEBUG_ASSERT(true, verbose_assert{}, "won't fire", 0);
    std::cout << "  Passed (extra args available but not triggered)" << std::endl;

    std::cout << std::endl;
}

// ============================================================================
// 第五部分：DEBUG_UNREACHABLE —— 标记不可达代码
// ============================================================================

struct unreachable_assert : debug_assert::set_level<1>, debug_assert::default_handler {};

enum class Color { Red, Green, Blue };

const char* color_name(Color c) {
    switch (c) {
        case Color::Red:
            return "Red";
        case Color::Green:
            return "Green";
        case Color::Blue:
            return "Blue";
    }
    // 理论上不可达，但编译器可能警告"not all paths return a value"
    // 使用 DEBUG_UNREACHABLE 标记，若到达则触发 handler
    DEBUG_UNREACHABLE(unreachable_assert{}, "unknown color");
    return "???";
}

// 也可用在 constexpr 三目表达式中
constexpr int safe_div(int a, int b) {
    return b != 0 ? a / b : (DEBUG_UNREACHABLE(unreachable_assert{}, "division by zero"), 0);
}

void demo_unreachable() {
    std::cout << "=== 第五部分：DEBUG_UNREACHABLE ===" << std::endl;

    std::cout << "  color_name(Red)   = " << color_name(Color::Red) << std::endl;
    std::cout << "  color_name(Green) = " << color_name(Color::Green) << std::endl;
    std::cout << "  color_name(Blue)  = " << color_name(Color::Blue) << std::endl;

    constexpr int result = safe_div(10, 2);
    std::cout << "  safe_div(10, 2) = " << result << std::endl;

    std::cout << std::endl;
}

// ============================================================================
// 第六部分：实战 —— 每个模块独立的断言控制
// ============================================================================

// 模拟项目中不同模块使用各自独立的断言级别

// 网络模块：生产环境关闭断言 (level=0)
struct net_assert : debug_assert::set_level<0>, debug_assert::default_handler {};

// 解析模块：保留基础断言 (level=1)
struct parser_assert : debug_assert::set_level<1>, debug_assert::default_handler {};

// 调试模块：开启所有断言 (level=3)
struct debug_module_assert : debug_assert::set_level<3>, debug_assert::default_handler {};

void simulate_network(int port) {
    // level=0 → 所有断言都被编译期移除，零开销
    DEBUG_ASSERT(port > 0 && port < 65536, net_assert{}, "invalid port");
    std::cout << "  [net] listening on port " << port << " (assert compiled out)" << std::endl;
}

void simulate_parse(const std::string& input) {
    DEBUG_ASSERT(!input.empty(), parser_assert{}, "input must not be empty");
    std::cout << "  [parser] parsing: " << input << std::endl;
}

void simulate_debug(int value) {
    DEBUG_ASSERT(value >= 0, debug_module_assert{}, debug_assert::level<1>{}, "non-negative");
    DEBUG_ASSERT(value < 1000, debug_module_assert{}, debug_assert::level<2>{}, "< 1000");
    DEBUG_ASSERT(value % 2 == 0, debug_module_assert{}, debug_assert::level<3>{}, "must be even");
    std::cout << "  [debug] value " << value << " passed all checks" << std::endl;
}

void demo_per_module() {
    std::cout << "=== 第六部分：模块独立的断言控制 ===" << std::endl;

    simulate_network(8080);
    simulate_parse("hello world");
    simulate_debug(42);

    std::cout << std::endl;
}

// ============================================================================
// 第七部分：使用可变参数模式，测试在 handle 中使用 fmt
// ============================================================================

// 调用链路：
//   DEBUG_ASSERT(expr, handler{}, fmt_str, arg1, arg2, ...)
//   → handle(loc, "expr", fmt_str, arg1, arg2, ...)
//
// handle 的前两个参数固定是 (loc, expression)，之后全是我们传的额外参数。
// 所以可以直接用模板可变参数：第三个参数当 format string，其余当 format args。

struct fmt_assert : debug_assert::set_level<1>, debug_assert::allow_exception {
    // 无额外参数：纯表达式断言
    static void handle(const debug_assert::source_location& loc, const char* expression) {
        auto full = std::format("[fmt assert] {}:{}: '{}' failed\n", loc.file_name, loc.line_number,
                                expression);
        std::fprintf(stderr, "%s", full.c_str());
        throw std::runtime_error(full);
    }

    // 带 format string + 可变参数
    // 注意：不能用 std::format_string<Args...>，因为 debug_assert 库内部通过
    // detail::forward 转发参数，format string 已变成运行时值，
    // 无法满足 std::format_string 的 consteval 构造要求。
    // 改用 std::vformat + std::make_format_args 在运行时格式化。
    template <typename... Args>
    static void handle(const debug_assert::source_location& loc, const char* expression,
                       const char* fmt, Args&&... args) {
        auto message = std::vformat(fmt, std::make_format_args(args...));
        auto full    = std::format("[fmt assert] {}:{}: '{}' failed - {}\n", loc.file_name,
                                   loc.line_number, expression, message);
        std::fprintf(stderr, "%s", full.c_str());
        throw std::runtime_error(full);
    }
};

void demo_fmt() {
    std::cout << "=== 第七部分：配合 std::format ===" << std::endl;

    int    a = 1;
    double b = 3.14;

    // 用法1：无消息
    try {
        DEBUG_ASSERT(a == 2, fmt_assert{});
    } catch (const std::runtime_error& e) {
        std::cout << "  Caught: " << e.what();
    }

    // 用法2：format string + 参数，直接传给 DEBUG_ASSERT
    try {
        DEBUG_ASSERT(a == 2, fmt_assert{}, "expected a={} to equal 2, pi={:.2f}", a, b);
    } catch (const std::runtime_error& e) {
        std::cout << "  Caught: " << e.what();
    }

    // 用法3：更多参数
    try {
        std::string name = "test";
        DEBUG_ASSERT(a > 100, fmt_assert{}, "name={}, a={}, b={:.1f}", name, a, b);
    } catch (const std::runtime_error& e) {
        std::cout << "  Caught: " << e.what();
    }

    std::cout << std::endl;
}
// ============================================================================

int main() {
    demo_basic();
    demo_levels();
    demo_throwing_handler();
    demo_extra_args();
    demo_unreachable();
    demo_per_module();
    demo_fmt();

    std::cout << "=== 总结 ===" << std::endl;
    std::cout << "  1. 定义 Handler: 继承 set_level<N> + handler 基类" << std::endl;
    std::cout << "  2. DEBUG_ASSERT(expr, handler{}, [level<N>{}], [args...])" << std::endl;
    std::cout << "  3. DEBUG_UNREACHABLE(handler{}, [level<N>{}], [args...])" << std::endl;
    std::cout << "  4. Handler::level >= N 时断言生效，否则编译期移除" << std::endl;
    std::cout << "  5. 每个模块独立 Handler，互不影响" << std::endl;

    return 0;
}
