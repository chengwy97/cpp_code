// iceoryx error_reporting 使用示例
//
// 覆盖内容：
//   ① 自定义模块错误类型（Error class + iox::er 命名空间特化）
//   ② IOX_REPORT           —— 非致命错误上报
//   ③ IOX_REPORT_IF        —— 条件非致命错误上报
//   ④ 自定义 ErrorHandler  —— 运行期替换 handler，记录错误
//   ⑤ IOX_REPORT_FATAL     —— 致命错误（借助抛出异常的 handler 演示）
//   ⑥ IOX_REPORT_FATAL_IF  —— 条件致命错误
//   ⑦ handler 的安装 / 重置 / finalize 生命周期

// 必须先引入模块错误类型，再引入上报 API
// macros.hpp 引入了完整的上报宏（IOX_REPORT / IOX_REPORT_FATAL 等）
// 它内部依赖 error_forwarding.hpp → custom/error_reporting.hpp，
// 因此只需一个入口头即可
#include "iox/error_reporting/macros.hpp"

// 替换 handler 用
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "iox/error_reporting/custom/default/error_handler.hpp"
#include "iox/static_lifetime_guard.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// § 1  自定义模块错误类型
//
// 每个模块需要：
//   a. 定义 Code 枚举 + Error 类（满足 code()/module()/name()/moduleName() 接口）
//   b. 在 iox::er 命名空间里提供 toError / toModule 重载（ADL 查找需要）
// ─────────────────────────────────────────────────────────────────────────────

namespace network_module {
namespace errors {

using ErrorCode = iox::er::ErrorCode;
using ModuleId  = iox::er::ModuleId;

// 用户模块 ID 须 >= ModuleId::USER_MODULE_BASE(0x100) 以避免与内置模块冲突
constexpr ModuleId MODULE_ID{0x200};

// NOLINTNEXTLINE(performance-enum-size)
enum class Code : ErrorCode::type {
    Timeout           = 1,
    ConnectionRefused = 2,
    InvalidAddress    = 3,
    OutOfBuffer       = 4,
};

inline const char* asStringLiteral(Code code) {
    switch (code) {
        case Code::Timeout:
            return "Timeout";
        case Code::ConnectionRefused:
            return "ConnectionRefused";
        case Code::InvalidAddress:
            return "InvalidAddress";
        case Code::OutOfBuffer:
            return "OutOfBuffer";
    }
    return "unknown";
}

class Error {
   public:
    constexpr explicit Error(Code code) : m_code(code) {}

    ErrorCode code() const { return ErrorCode(static_cast<ErrorCode::type>(m_code)); }

    static constexpr ModuleId module() { return MODULE_ID; }

    static const char* moduleName() { return "network_module"; }

    const char* name() const { return asStringLiteral(m_code); }

   private:
    Code m_code;
};

}  // namespace errors
}  // namespace network_module

// ── iox::er 命名空间特化：让宏能找到正确的 toError/toModule ─────────────────
namespace iox {
namespace er {

inline network_module::errors::Error toError(network_module::errors::Code code) {
    return network_module::errors::Error{code};
}

inline ModuleId toModule(network_module::errors::Code) {
    return network_module::errors::MODULE_ID;
}

template <>
inline const char* toModuleName<network_module::errors::Error>(
    const network_module::errors::Error& e) {
    return e.moduleName();
}

template <>
inline const char* toErrorName<network_module::errors::Error>(
    const network_module::errors::Error& e) {
    return e.name();
}

}  // namespace er
}  // namespace iox

// ─────────────────────────────────────────────────────────────────────────────
// § 2  自定义 ErrorHandler 实现
// ─────────────────────────────────────────────────────────────────────────────

// ---- ② RecordingHandler：记录每次错误，不终止进程 ----
struct ErrorRecord {
    iox::er::ErrorCode code;
    iox::er::ModuleId  module;
    std::string        file;
    int                line;
};

class RecordingHandler : public iox::er::ErrorHandlerInterface {
   public:
    void onPanic() override {
        // 致命错误：默认应 abort，这里仅记录（用于非致命演示场景）
        std::cerr << "[RecordingHandler] onPanic called (would abort in production)\n";
    }

    void onReportError(iox::er::ErrorDescriptor desc) override {
        errors.push_back({desc.code, desc.module, desc.location.file, desc.location.line});
        std::cout << "[RecordingHandler] Error recorded: code=" << desc.code.value
                  << " module=" << desc.module.value << " at " << desc.location.file << ":"
                  << desc.location.line << "\n";
    }

    void onReportViolation(iox::er::ErrorDescriptor desc) override {
        std::cout << "[RecordingHandler] Violation: code=" << desc.code.value << "\n";
    }

    std::vector<ErrorRecord> errors;
};

// ---- ③ ThrowingHandler：onPanic 抛出异常，用于演示致命错误而不真的 abort ----
struct PanicSignal {};  // 轻量异常类型，代表 panic 信号

class ThrowingHandler : public iox::er::ErrorHandlerInterface {
   public:
    void onPanic() override {
        // [[noreturn]] 仅是编译器提示；在测试/演示中抛出是常用技巧
        throw PanicSignal{};
    }

    void onReportError(iox::er::ErrorDescriptor desc) override {
        // kind = FATAL 时走这里
        std::cout << "  [ThrowingHandler::onReportError    ] code=" << desc.code.value
                  << " module=" << desc.module.value << "\n";
    }

    void onReportViolation(iox::er::ErrorDescriptor desc) override {
        // kind = ASSERT_VIOLATION / ENFORCE_VIOLATION 时走这里
        std::cout << "  [ThrowingHandler::onReportViolation] code=" << desc.code.value
                  << " module=" << desc.module.value << "\n";
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// § 3  各 Demo 函数
// ─────────────────────────────────────────────────────────────────────────────

using Code = network_module::errors::Code;
using Handler =
    iox::er::ErrorHandler;  // PolymorphicHandler<ErrorHandlerInterface, DefaultErrorHandler>

// ─── Demo 1: IOX_REPORT —— 非致命错误上报 ────────────────────────────────────
void demo_report() {
    std::cout << "\n=== DEMO 1: IOX_REPORT (非致命, RUNTIME_ERROR) ===\n";

    // 直接上报错误码（iox::er::toError 会把 Code 转为 Error 对象）
    IOX_REPORT(Code::Timeout, iox::er::RUNTIME_ERROR);
    std::cout << "  执行继续（非致命，不终止进程）\n";

    IOX_REPORT(Code::ConnectionRefused, iox::er::RUNTIME_ERROR);
    std::cout << "  再次继续\n";

    // 也可以直接传 Error 对象
    IOX_REPORT(network_module::errors::Error{Code::InvalidAddress}, iox::er::RUNTIME_ERROR);
    std::cout << "  传 Error 对象也可以\n";
}

// ─── Demo 2: IOX_REPORT_IF —— 条件非致命上报 ────────────────────────────────
void demo_report_if() {
    std::cout << "\n=== DEMO 2: IOX_REPORT_IF (条件非致命) ===\n";

    int bufferUsed  = 90;
    int bufferTotal = 100;

    // 条件为 false → 不触发
    IOX_REPORT_IF(bufferUsed > bufferTotal, Code::OutOfBuffer, iox::er::RUNTIME_ERROR);
    std::cout << "  bufferUsed=" << bufferUsed << " <= total=" << bufferTotal << "  → 未触发\n";

    bufferUsed = 101;

    // 条件为 true → 触发，日志中能看到被字符串化的条件表达式
    IOX_REPORT_IF(bufferUsed > bufferTotal, Code::OutOfBuffer, iox::er::RUNTIME_ERROR);
    std::cout << "  bufferUsed=" << bufferUsed << " > total=" << bufferTotal
              << "  → 已触发，继续执行\n";
}

// ─── Demo 3: 自定义 RecordingHandler ─────────────────────────────────────────
void demo_custom_handler() {
    std::cout << "\n=== DEMO 3: 自定义 RecordingHandler ===\n";

    // StaticLifetimeGuard 确保自定义 handler 对象在 set 期间一直存活
    // Handler::set 接收一个 guard 的拷贝（轻量引用计数对象）
    Handler::set(iox::StaticLifetimeGuard<RecordingHandler>());

    IOX_REPORT(Code::Timeout, iox::er::RUNTIME_ERROR);
    IOX_REPORT(Code::ConnectionRefused, iox::er::RUNTIME_ERROR);
    IOX_REPORT(Code::InvalidAddress, iox::er::RUNTIME_ERROR);

    // 读取 RecordingHandler 中的记录
    auto& h = static_cast<RecordingHandler&>(Handler::get());
    std::cout << "  共记录 " << h.errors.size() << " 条错误\n";
    for (const auto& r : h.errors) {
        std::cout << "    code=" << r.code.value << "  module=" << r.module.value << "  @ "
                  << r.file << ":" << r.line << "\n";
    }

    // 恢复默认 handler
    Handler::reset();
    std::cout << "  Handler 已重置为 Default\n";
}

// ─── Demo 4: IOX_REPORT_FATAL / IOX_REPORT_FATAL_IF / Violation ──────────────
//
// handler 回调路由：
//
//   kind = FATAL               → report(FatalKind)     → onReportError  + panic
//   kind = ASSERT_VIOLATION    → detail::report(...)   → onReportViolation + panic
//   kind = ENFORCE_VIOLATION   → detail::report(...)   → onReportViolation + panic
//
// IOX_REPORT_FATAL 硬编码 FATAL，所以永远走 onReportError。
// 要触发 onReportViolation 必须直接调用 forwardFatalError 并传入
// ASSERT_VIOLATION 或 ENFORCE_VIOLATION。
void demo_fatal() {
    std::cout << "\n=== DEMO 4: FATAL / ASSERT_VIOLATION / ENFORCE_VIOLATION ===\n";
    std::cout << "  （安装 ThrowingHandler，onPanic 抛出 PanicSignal 以避免 abort）\n";

    Handler::set(iox::StaticLifetimeGuard<ThrowingHandler>());

    // ── ① IOX_REPORT_FATAL → kind=FATAL → onReportError ─────────────────
    try {
        IOX_REPORT_FATAL(Code::OutOfBuffer);
    } catch (const PanicSignal&) {
        std::cout << "  IOX_REPORT_FATAL  kind=FATAL           → onReportError ✓\n";
    }

    // ── ② ASSERT_VIOLATION → onReportViolation ───────────────────────────
    // IOX_REPORT_FATAL_IF 同样只用 FATAL；要走 onReportViolation
    // 必须手动调用 forwardFatalError 并指定 violation kind。
    try {
        iox::er::forwardFatalError(
            iox::er::toError(Code::InvalidAddress),
            iox::er::ASSERT_VIOLATION,  // AssertViolationKind → detail::report → onReportViolation
            IOX_CURRENT_SOURCE_LOCATION, "Code::InvalidAddress != Code::Timeout");
    } catch (const PanicSignal&) {
        std::cout << "  forwardFatalError  ASSERT_VIOLATION     → onReportViolation ✓\n";
    }

    // ── ③ ENFORCE_VIOLATION → onReportViolation（附带消息）──────────────
    try {
        iox::er::forwardFatalError(
            iox::er::toError(Code::Timeout),
            iox::er::ENFORCE_VIOLATION,  // EnforceViolationKind → detail::report →
                                         // onReportViolation
            IOX_CURRENT_SOURCE_LOCATION, "timeout_ms > 0",
            "timeout must be positive");  // 额外 Message 参数
    } catch (const PanicSignal&) {
        std::cout << "  forwardFatalError  ENFORCE_VIOLATION    → onReportViolation ✓\n";
    }

    // ── ④ IOX_REPORT_FATAL_IF 条件为 false → 不触发 ──────────────────────
    try {
        IOX_REPORT_FATAL_IF(false, Code::Timeout);
        std::cout << "  IOX_REPORT_FATAL_IF(false,...)         → 未触发 ✓\n";
    } catch (const PanicSignal&) {
        std::cout << "  [BUG] 不应到达这里\n";
    }

    Handler::reset();
    std::cout << "  Handler 已重置为 Default\n";
}

// ─── Demo 5: handler 生命周期（finalize）─────────────────────────────────────
void demo_lifecycle() {
    std::cout << "\n=== DEMO 5: PolymorphicHandler 生命周期 ===\n";

    // 安装 RecordingHandler
    bool ok1 = Handler::set(iox::StaticLifetimeGuard<RecordingHandler>());
    std::cout << "  set RecordingHandler: " << (ok1 ? "success" : "failed") << "\n";

    // finalize 后不允许再替换 handler
    Handler::finalize();
    std::cout << "  finalize() called\n";

    // 尝试再次替换 → 失败（DefaultHooks::onSetAfterFinalize 会 abort，
    //   但 PolymorphicHandler 内部会先检查并返回 false，不一定调用 hooks）
    // 注意：finalize 之后行为由 Hooks 定义，默认是 abort。
    // 这里仅展示 set 的返回值含义。
    std::cout << "  (不调用 set/reset：finalize 后调用可能触发 abort，见 DefaultHooks)\n";

    // 当前 handler 仍可正常使用
    IOX_REPORT(Code::Timeout, iox::er::RUNTIME_ERROR);
    std::cout << "  finalize 后仍可正常上报错误\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// § 4  自注册（self-registration）模式
//
// 目标：每个子模块只需在 .cpp 里定义一个静态对象，就自动把自己的 handler
//       注册进全局 DispatchHandler，无需任何显式调用。
//
// 关键机制：
//   1. DispatchHandler 内部用函数局部静态（Meyers 单例）保存路由表
//      → 避免 SIOF（静态初始化顺序问题）
//   2. ModuleRegistrar 的构造函数在 main() 前由静态初始化阶段自动运行
//      → 即"定义即注册"
//   3. ensureInstalled() 只安装一次，由第一个注册者触发
//      → 多模块重复调用安全
// ─────────────────────────────────────────────────────────────────────────────

class DispatchHandler : public iox::er::ErrorHandlerInterface {
   public:
    using SubHandler = std::function<void(iox::er::ErrorDescriptor)>;

    // ── 自注册 RAII 对象 ───────────────────────────────────────────────────
    // 在 .cpp 文件作用域定义一个静态 ModuleRegistrar 即自动完成注册。
    // 生命期与翻译单元静态变量相同（程序结束才析构）。
    struct ModuleRegistrar {
        ModuleRegistrar(iox::er::ModuleId id, SubHandler onError, SubHandler onViolation = {}) {
            // 使用函数局部静态的路由表（Meyers 单例），保证先于本构造体初始化
            errorTable()[id.value] = std::move(onError);
            if (onViolation) {
                violationTable()[id.value] = std::move(onViolation);
            }
            // 确保全局 handler 已替换为 DispatchHandler（只做一次）
            ensureInstalled();
        }
    };

    // ── 路由表（函数局部静态，线程安全初始化）────────────────────────────
    static std::unordered_map<uint32_t, SubHandler>& errorTable() {
        static std::unordered_map<uint32_t, SubHandler> m;
        return m;
    }

    static std::unordered_map<uint32_t, SubHandler>& violationTable() {
        static std::unordered_map<uint32_t, SubHandler> m;
        return m;
    }

    // ── 安装为全局 handler（只执行一次）──────────────────────────────────
    static void ensureInstalled() {
        // 函数局部静态：第一次调用时执行 lambda，之后跳过
        static bool installed = []() {
            iox::er::ErrorHandler::set(iox::StaticLifetimeGuard<DispatchHandler>());
            return true;
        }();
        (void)installed;
    }

    // ── ErrorHandlerInterface 实现 ────────────────────────────────────────
    void onPanic() override { std::abort(); }

    void onReportError(iox::er::ErrorDescriptor desc) override {
        dispatch(errorTable(), desc, "onReportError");
    }

    void onReportViolation(iox::er::ErrorDescriptor desc) override {
        // 先查 violation 专用表，找不到则退回到 error 表
        if (!dispatch(violationTable(), desc, "onReportViolation")) {
            dispatch(errorTable(), desc, "onReportViolation(fallback)");
        }
    }

   private:
    static bool dispatch(std::unordered_map<uint32_t, SubHandler>& table,
                         iox::er::ErrorDescriptor desc, const char* tag) {
        auto it = table.find(desc.module.value);
        if (it == table.end()) {
            std::cerr << "[DispatchHandler::" << tag
                      << "] no handler for module=" << desc.module.value << "\n";
            return false;
        }
        it->second(desc);
        return true;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// § 5  各子模块"定义即注册"
//
// 真实项目里这些静态变量放在各自模块的 .cpp 里，彼此完全独立。
// 这里用匿名 namespace 模拟多个翻译单元。
// ─────────────────────────────────────────────────────────────────────────────

namespace {

// ── network_module 自注册 ─────────────────────────────────────────────────
// 放在 network_module.cpp（此处模拟）：只要链接进来就自动注册，无需显式调用
static DispatchHandler::ModuleRegistrar s_networkReg{
    network_module::errors::MODULE_ID,
    // onError
    [](iox::er::ErrorDescriptor desc) {
        std::cout << "  [network handler] error  code=" << desc.code.value << "  @ "
                  << desc.location.file << ":" << desc.location.line << "\n";
    },
    // onViolation（可选）
    [](iox::er::ErrorDescriptor desc) {
        std::cout << "  [network handler] violation code=" << desc.code.value << "\n";
    }};

// ── storage_module 自注册 ─────────────────────────────────────────────────
// 另一个子模块，独立定义，独立注册
namespace storage_module {
namespace errors {
constexpr iox::er::ModuleId MODULE_ID{0x201};
// NOLINTNEXTLINE(performance-enum-size)
enum class Code : iox::er::ErrorCode::type { DiskFull = 1, PermissionDenied = 2 };
class Error {
   public:
    constexpr explicit Error(Code c) : m_code(c) {}
    iox::er::ErrorCode code() const {
        return iox::er::ErrorCode(static_cast<iox::er::ErrorCode::type>(m_code));
    }
    static constexpr iox::er::ModuleId module() { return MODULE_ID; }
    static const char*                 moduleName() { return "storage_module"; }
    const char* name() const { return m_code == Code::DiskFull ? "DiskFull" : "PermissionDenied"; }

   private:
    Code m_code;
};
}  // namespace errors
}  // namespace storage_module
}  // namespace

namespace iox {
namespace er {
inline storage_module::errors::Error toError(storage_module::errors::Code c) {
    return storage_module::errors::Error{c};
}
// NOLINTNEXTLINE(misc-unused-using-decls,clang-diagnostic-unused-function)
inline ModuleId toModule(storage_module::errors::Code)  // NOLINT(misc-unused-using-decls)
{
    return storage_module::errors::MODULE_ID;
}
// 以下两个特化通过模板 ADL 间接调用，编译器静态分析认为"未使用"属误报
template <>
inline const char* toModuleName<storage_module::errors::Error>(
    const storage_module::errors::Error& e) {
    return e.moduleName();
}  // NOLINT
template <>
inline const char* toErrorName<storage_module::errors::Error>(
    const storage_module::errors::Error& e) {
    return e.name();
}  // NOLINT
}  // namespace er
}  // namespace iox

namespace {
static DispatchHandler::ModuleRegistrar s_storageReg{
    storage_module::errors::MODULE_ID, [](iox::er::ErrorDescriptor desc) {
        std::cout << "  [storage handler] error  code=" << desc.code.value << "  @ "
                  << desc.location.file << ":" << desc.location.line << "\n";
    }};
}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Demo 6: 自注册 + 按模块路由
// ─────────────────────────────────────────────────────────────────────────────

void demo_self_registration() {
    std::cout << "\n=== DEMO 6: 自注册 + DispatchHandler 按模块路由 ===\n";
    std::cout << "  （s_networkReg / s_storageReg 在 main() 前已完成注册）\n\n";

    // 直接上报，DispatchHandler 按 ModuleId 分发，无需任何显式调用
    IOX_REPORT(Code::Timeout, iox::er::RUNTIME_ERROR);
    IOX_REPORT(Code::ConnectionRefused, iox::er::RUNTIME_ERROR);
    IOX_REPORT(storage_module::errors::Code::DiskFull, iox::er::RUNTIME_ERROR);
    IOX_REPORT(storage_module::errors::Code::PermissionDenied, iox::er::RUNTIME_ERROR);

    std::cout << "\n  每条错误都被路由到各自模块的 handler ✓\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main() {
    // Demo 1-5 先重置，避免被 s_networkReg / s_storageReg 提前安装的 DispatchHandler 干扰
    iox::er::ErrorHandler::reset();

    demo_report();
    demo_report_if();
    demo_custom_handler();
    demo_fatal();
    demo_lifecycle();

    // Demo 6：自注册，重新安装 DispatchHandler
    iox::er::ErrorHandler::reset();
    DispatchHandler::ensureInstalled();
    demo_self_registration();

    std::cout << "\n=== 全部 Demo 完成 ===\n";
    return 0;
}
