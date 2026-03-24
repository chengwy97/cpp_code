// ============================================================================
// 模板策略模式 (Template Policy Pattern) 入门教学
//
// 策略模式的核心思想：将算法/行为封装为独立的"策略类"，在编译期通过模板参数注入，
// 实现零开销的多态。相比虚函数的运行时多态，策略模式在编译期决议，无虚表开销。
// ============================================================================

#include <cmath>
#include <concepts>
#include <iostream>
#include <string>
#include <vector>

// ============================================================================
// 第一部分：基础策略模式 —— 日志策略
// 展示如何通过模板参数注入不同的日志行为
// ============================================================================

// 策略1：输出到控制台
struct ConsoleLogPolicy {
    static void log(const std::string& msg) { std::cout << "[Console] " << msg << std::endl; }
};

// 策略2：静默（不输出）
struct NullLogPolicy {
    static void log(const std::string& /*msg*/) {
        // 什么都不做
    }
};

// 策略3：带前缀的日志
struct PrefixLogPolicy {
    static void log(const std::string& msg) { std::cout << "[PREFIX][LOG] " << msg << std::endl; }
};

// 使用策略的"宿主类"：通过模板参数选择日志策略
template <typename LogPolicy>
class Service {
   public:
    void do_work() {
        LogPolicy::log("Service started");
        // ... 业务逻辑 ...
        LogPolicy::log("Service finished");
    }
};

void demo_log_policy() {
    std::cout << "=== 第一部分：日志策略 ===" << std::endl;

    Service<ConsoleLogPolicy> verbose_service;
    verbose_service.do_work();

    std::cout << std::endl;

    Service<NullLogPolicy> silent_service;
    silent_service.do_work();  // 不会输出任何日志

    std::cout << std::endl;

    Service<PrefixLogPolicy> prefix_service;
    prefix_service.do_work();

    std::cout << std::endl;
}

// ============================================================================
// 第二部分：多策略组合 —— 排序器
// 展示如何组合多个策略维度
// ============================================================================

// 维度1：比较策略
struct AscendingCompare {
    static bool        compare(int a, int b) { return a < b; }
    static const char* name() { return "升序"; }
};

struct DescendingCompare {
    static bool        compare(int a, int b) { return a > b; }
    static const char* name() { return "降序"; }
};

// 维度2：输出策略
struct InlineOutput {
    static void print(const std::vector<int>& data) {
        for (auto v : data) std::cout << v << " ";
        std::cout << std::endl;
    }
};

struct BracketOutput {
    static void print(const std::vector<int>& data) {
        std::cout << "[";
        for (std::size_t i = 0; i < data.size(); ++i) {
            if (i > 0)
                std::cout << ", ";
            std::cout << data[i];
        }
        std::cout << "]" << std::endl;
    }
};

// 组合两个策略维度的宿主类
template <typename ComparePolicy, typename OutputPolicy>
class Sorter {
   public:
    void sort_and_print(std::vector<int> data) {
        std::cout << ComparePolicy::name() << "排序: ";

        // 使用比较策略进行冒泡排序
        for (std::size_t i = 0; i < data.size(); ++i) {
            for (std::size_t j = i + 1; j < data.size(); ++j) {
                if (ComparePolicy::compare(data[j], data[i])) {
                    std::swap(data[i], data[j]);
                }
            }
        }

        // 使用输出策略打印结果
        OutputPolicy::print(data);
    }
};

void demo_multi_policy() {
    std::cout << "=== 第二部分：多策略组合 ===" << std::endl;

    std::vector<int> data = {5, 3, 8, 1, 9, 2, 7};

    Sorter<AscendingCompare, InlineOutput>   s1;
    Sorter<DescendingCompare, BracketOutput> s2;
    Sorter<AscendingCompare, BracketOutput>  s3;

    s1.sort_and_print(data);
    s2.sort_and_print(data);
    s3.sort_and_print(data);

    std::cout << std::endl;
}

// ============================================================================
// 第三部分：带状态的策略 + concepts 约束
// 展示策略类可以持有状态，同时用 concepts 约束策略接口
// ============================================================================

// 用 concept 定义策略接口约束
template <typename T>
concept DiscountPolicy = requires(T policy, double price) {
    { policy.apply(price) } -> std::convertible_to<double>;
    { T::name() } -> std::convertible_to<const char*>;
};

// 策略1：无折扣
struct NoDiscount {
    double             apply(double price) const { return price; }
    static const char* name() { return "无折扣"; }
};

// 策略2：固定百分比折扣（带状态）
class PercentDiscount {
    double rate_;  // 例如 0.2 表示打八折

   public:
    explicit PercentDiscount(double rate) : rate_(rate) {}
    double             apply(double price) const { return price * (1.0 - rate_); }
    static const char* name() { return "百分比折扣"; }
};

// 策略3：满减（带状态）
class ThresholdDiscount {
    double threshold_;
    double reduction_;

   public:
    ThresholdDiscount(double threshold, double reduction)
        : threshold_(threshold), reduction_(reduction) {}

    double apply(double price) const { return price >= threshold_ ? price - reduction_ : price; }
    static const char* name() { return "满减折扣"; }
};

// 宿主类：用 concept 约束策略
template <DiscountPolicy Policy>
class PriceCalculator {
    Policy policy_;

   public:
    explicit PriceCalculator(Policy policy = {}) : policy_(std::move(policy)) {}

    void calculate(double original_price) {
        double final_price = policy_.apply(original_price);
        std::cout << Policy::name() << ": " << original_price << " -> " << final_price << std::endl;
    }
};

void demo_stateful_policy() {
    std::cout << "=== 第三部分：带状态的策略 + concepts ===" << std::endl;

    PriceCalculator<NoDiscount> calc1;
    calc1.calculate(100.0);

    PriceCalculator calc2(PercentDiscount(0.2));  // CTAD 自动推导模板参数
    calc2.calculate(100.0);

    PriceCalculator calc3(ThresholdDiscount(80.0, 15.0));  // 满80减15
    calc3.calculate(100.0);
    calc3.calculate(50.0);

    std::cout << std::endl;
}

// ============================================================================
// 第四部分：CRTP (Curiously Recurring Template Pattern)
// 策略模式的变体：基类通过模板参数"认识"派生类，实现编译期多态
// ============================================================================

template <typename Derived>
class Shape {
   public:
    void draw() const {
        // 编译期多态：调用派生类的 draw_impl
        static_cast<const Derived*>(this)->draw_impl();
    }

    double area() const { return static_cast<const Derived*>(this)->area_impl(); }

    void info() const {
        std::cout << "Shape: area = " << area() << std::endl;
        draw();
    }
};

class Circle : public Shape<Circle> {
    double radius_;

   public:
    explicit Circle(double r) : radius_(r) {}
    void   draw_impl() const { std::cout << "  Drawing circle (r=" << radius_ << ")" << std::endl; }
    double area_impl() const { return M_PI * radius_ * radius_; }
};

class Rectangle : public Shape<Rectangle> {
    double w_, h_;

   public:
    Rectangle(double w, double h) : w_(w), h_(h) {}
    void draw_impl() const {
        std::cout << "  Drawing rectangle (" << w_ << "x" << h_ << ")" << std::endl;
    }
    double area_impl() const { return w_ * h_; }
};

// 模板函数接受任意 Shape<Derived>
template <typename Derived>
void print_shape_info(const Shape<Derived>& shape) {
    shape.info();
}

void demo_crtp() {
    std::cout << "=== 第四部分：CRTP 编译期多态 ===" << std::endl;

    Circle    c(5.0);
    Rectangle r(3.0, 4.0);

    print_shape_info(c);
    print_shape_info(r);

    std::cout << std::endl;
}

// ============================================================================
// 第五部分：子类继承多个 Policy 父类 (Mixin-based Policy Design)
//
// 核心思想：宿主类同时继承多个策略基类，每个策略提供一个独立的能力切面。
// 子类通过多重继承将多个策略"混入"自身，像搭积木一样组合出完整功能。
//
//  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐
//  │ ThreadPolicy │  │ LogPolicy    │  │ ValidPolicy  │
//  │  lock/unlock │  │  log(msg)    │  │  validate()  │
//  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘
//         │                 │                 │
//         └─────────────────┼─────────────────┘
//                           │
//                  ┌────────▼────────┐
//                  │   Repository    │
//                  │ (继承三个策略)  │
//                  └─────────────────┘
// ============================================================================

// --- 策略维度1：线程安全策略 ---

struct NoLock {
    void               lock() {}
    void               unlock() {}
    static const char* thread_policy_name() { return "NoLock"; }
};

struct MutexLock {
    void               lock() { std::cout << "    [mutex locked]" << std::endl; }
    void               unlock() { std::cout << "    [mutex unlocked]" << std::endl; }
    static const char* thread_policy_name() { return "MutexLock"; }
};

// --- 策略维度2：日志策略 ---

struct StdoutLog {
    void log(const std::string& msg) const { std::cout << "    [log] " << msg << std::endl; }
    static const char* log_policy_name() { return "StdoutLog"; }
};

struct SilentLog {
    void               log(const std::string& /*msg*/) const {}
    static const char* log_policy_name() { return "SilentLog"; }
};

// --- 策略维度3：校验策略 ---

struct StrictValidation {
    bool validate(int value) const {
        bool ok = (value > 0 && value < 1000);
        if (!ok)
            std::cout << "    [validate] REJECTED: " << value << std::endl;
        return ok;
    }
    static const char* validation_policy_name() { return "Strict"; }
};

struct NoValidation {
    bool               validate(int /*value*/) const { return true; }
    static const char* validation_policy_name() { return "None"; }
};

// --- 宿主类：继承三个策略基类 ---
// 通过 public 继承，宿主类获得每个策略的成员函数，可以直接调用。

template <typename ThreadPolicy, typename LogPolicy, typename ValidationPolicy>
class Repository : public ThreadPolicy, public LogPolicy, public ValidationPolicy {
    std::vector<int> data_;

   public:
    void describe() const {
        std::cout << "  Repository<" << ThreadPolicy::thread_policy_name() << ", "
                  << LogPolicy::log_policy_name() << ", "
                  << ValidationPolicy::validation_policy_name() << ">" << std::endl;
    }

    bool add(int value) {
        // 使用校验策略
        if (!this->validate(value)) {
            this->log("Validation failed for value: " + std::to_string(value));
            return false;
        }

        // 使用线程安全策略
        this->lock();
        data_.push_back(value);
        this->unlock();

        // 使用日志策略
        this->log("Added value: " + std::to_string(value));
        return true;
    }

    void print() const {
        std::cout << "  data: [";
        for (std::size_t i = 0; i < data_.size(); ++i) {
            if (i > 0)
                std::cout << ", ";
            std::cout << data_[i];
        }
        std::cout << "]" << std::endl;
    }
};

// 用 type alias 定义常用组合，简化使用
using SafeVerboseRepo = Repository<MutexLock, StdoutLog, StrictValidation>;
using FastSilentRepo  = Repository<NoLock, SilentLog, NoValidation>;
using SafeStrictRepo  = Repository<MutexLock, StdoutLog, NoValidation>;

void demo_mixin_policy() {
    std::cout << "=== 第五部分：子类继承多个 Policy 父类 ===" << std::endl;

    std::cout << "\n-- 安全+日志+严格校验 --" << std::endl;
    SafeVerboseRepo repo1;
    repo1.describe();
    repo1.add(42);
    repo1.add(-1);  // 被校验拒绝
    repo1.add(999);
    repo1.add(1001);  // 被校验拒绝
    repo1.print();

    std::cout << "\n-- 无锁+静默+无校验（最快） --" << std::endl;
    FastSilentRepo repo2;
    repo2.describe();
    repo2.add(42);
    repo2.add(-1);  // 无校验，直接通过
    repo2.add(1001);
    repo2.print();

    std::cout << std::endl;
}

// ============================================================================
// 第六部分：可变参数模板 —— 策略自由组合
//
// 核心思想：用 variadic template 让宿主类接受任意数量、任意顺序的策略。
// 没有传入的策略自动退化为默认(空)行为，传入的策略通过继承混入。
//
// 用法对比：
//   固定参数: Repository<MutexLock, StdoutLog, StrictValidation>  -- 必须写全
//   可变参数: PluginHost<MutexLock, StdoutLog>                    -- 按需选配
//            PluginHost<StdoutLog>                                -- 只要日志
//            PluginHost<>                                         -- 全部默认
// ============================================================================

// --- 策略接口约定 ---
// 每个策略只需提供约定的方法，宿主类通过 if constexpr + 类型萃取决定是否调用。

// 通过标签类型区分不同策略维度
struct LockTag {};
struct LogTag {};
struct ValidateTag {};

// 默认策略（空操作）
struct DefaultLock {
    using policy_tag = LockTag;
    void lock() {}
    void unlock() {}
};

struct DefaultLog {
    using policy_tag = LogTag;
    void log(const std::string& /*msg*/) const {}
};

struct DefaultValidate {
    using policy_tag = ValidateTag;
    bool validate(int /*value*/) const { return true; }
};

// 具体策略实现

struct SpinLock {
    using policy_tag = LockTag;
    void lock() { std::cout << "    [spinlock locked]" << std::endl; }
    void unlock() { std::cout << "    [spinlock unlocked]" << std::endl; }
};

struct ColorLog {
    using policy_tag = LogTag;
    void log(const std::string& msg) const {
        std::cout << "    \033[32m[log]\033[0m " << msg << std::endl;
    }
};

struct RangeValidate {
    using policy_tag = ValidateTag;
    bool validate(int value) const {
        bool ok = (value >= 0 && value <= 100);
        if (!ok)
            std::cout << "    [range] REJECTED: " << value << std::endl;
        return ok;
    }
};

// --- 策略查找器：从参数包中按 Tag 查找策略，找不到则用默认值 ---

// 主模板：只声明形状，不提供实现
template <typename Tag, typename Default, typename... Policies>
struct FindPolicy;

// 特化1（递归终止）：参数包为空，返回 Default
template <typename Tag, typename Default>
struct FindPolicy<Tag, Default> {
    using type = Default;
};

// 特化2（递归展开）：取出头部 Head，匹配则返回 Head，否则继续递归 Tail...
template <typename Tag, typename Default, typename Head, typename... Tail>
struct FindPolicy<Tag, Default, Head, Tail...> {
    using type = std::conditional_t<std::is_same_v<typename Head::policy_tag, Tag>, Head,
                                    typename FindPolicy<Tag, Default, Tail...>::type>;
};

template <typename Tag, typename Default, typename... Policies>
using FindPolicy_t = typename FindPolicy<Tag, Default, Policies...>::type;

// --- 宿主类：继承从参数包中解析出的策略 ---

template <typename... Policies>
class PluginHost : public FindPolicy_t<LockTag, DefaultLock, Policies...>,
                   public FindPolicy_t<LogTag, DefaultLog, Policies...>,
                   public FindPolicy_t<ValidateTag, DefaultValidate, Policies...> {
    using LockP     = FindPolicy_t<LockTag, DefaultLock, Policies...>;
    using LogP      = FindPolicy_t<LogTag, DefaultLog, Policies...>;
    using ValidateP = FindPolicy_t<ValidateTag, DefaultValidate, Policies...>;

    std::vector<int> data_;

   public:
    bool add(int value) {
        if (!ValidateP::validate(value)) {
            LogP::log("Validation failed: " + std::to_string(value));
            return false;
        }

        LockP::lock();
        data_.push_back(value);
        LockP::unlock();

        LogP::log("Added: " + std::to_string(value));
        return true;
    }

    void print() const {
        std::cout << "  data: [";
        for (std::size_t i = 0; i < data_.size(); ++i) {
            if (i > 0)
                std::cout << ", ";
            std::cout << data_[i];
        }
        std::cout << "]" << std::endl;
    }
};

void demo_variadic_policy() {
    std::cout << "=== 第六部分：可变参数模板 —— 策略自由组合 ===" << std::endl;

    // 全部默认：无锁、无日志、无校验
    std::cout << "\n-- PluginHost<>（全部默认）--" << std::endl;
    PluginHost<> h1;
    h1.add(42);
    h1.add(-1);
    h1.print();

    // 只选日志
    std::cout << "\n-- PluginHost<ColorLog>（只要日志）--" << std::endl;
    PluginHost<ColorLog> h2;
    h2.add(42);
    h2.add(-1);
    h2.print();

    // 选锁 + 校验，不要日志
    std::cout << "\n-- PluginHost<SpinLock, RangeValidate>（锁+校验）--" << std::endl;
    PluginHost<SpinLock, RangeValidate> h3;
    h3.add(42);
    h3.add(200);  // 被校验拒绝
    h3.add(50);
    h3.print();

    // 全选：任意顺序
    std::cout << "\n-- PluginHost<RangeValidate, ColorLog, SpinLock>（全选，任意顺序）--"
              << std::endl;
    PluginHost<RangeValidate, ColorLog, SpinLock> h4;
    h4.add(10);
    h4.add(999);  // 被校验拒绝
    h4.add(88);
    h4.print();

    std::cout << std::endl;
}

// ============================================================================
// 第七部分：通过策略控制子类的可拷贝性与可比较性
//
// 三种常见手法：
//   1. 继承一个 deleted/defaulted 拷贝构造的基类（Mixin 方式）
//   2. 通过 CRTP + 策略注入 operator== / operator<=>
//   3. 用 concepts 在编译期约束"是否可比较"
// ============================================================================

// ---- 手法1：Mixin 控制拷贝 ----

struct Copyable {
    Copyable()                           = default;
    Copyable(const Copyable&)            = default;
    Copyable& operator=(const Copyable&) = default;
    Copyable(Copyable&&)                 = default;
    Copyable& operator=(Copyable&&)      = default;
};

struct NonCopyable {
    NonCopyable()                              = default;
    NonCopyable(const NonCopyable&)            = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
    NonCopyable(NonCopyable&&)                 = default;
    NonCopyable& operator=(NonCopyable&&)      = default;
};

struct NonMovable {
    NonMovable()                             = default;
    NonMovable(const NonMovable&)            = delete;
    NonMovable& operator=(const NonMovable&) = delete;
    NonMovable(NonMovable&&)                 = delete;
    NonMovable& operator=(NonMovable&&)      = delete;
};

// ---- 手法2：Mixin 控制比较（template-template 参数统一接口） ----

// 不可比较
template <typename /*Derived*/>
struct NotComparablePolicy {};

// 仅支持 == / !=
template <typename Derived>
struct EqualityComparable {
    friend bool operator==(const Derived& a, const Derived& b) {
        return a.compare_key() == b.compare_key();
    }
    friend bool operator!=(const Derived& a, const Derived& b) {
        return !(a == b);
    }
};

// 支持全序比较 <, <=, >, >=, ==, != (C++20 spaceship)
template <typename Derived>
struct FullyOrdered {
    friend auto operator<=>(const Derived& a, const Derived& b) {
        return a.compare_key() <=> b.compare_key();
    }
    friend bool operator==(const Derived& a, const Derived& b) {
        return a.compare_key() == b.compare_key();
    }
};

// ---- 宿主类：通过模板参数选择拷贝策略和比较策略 ----

template <typename CopyPolicy, template <typename> class ComparePolicy = EqualityComparable>
class Token : public CopyPolicy, public ComparePolicy<Token<CopyPolicy, ComparePolicy>> {
    int    id_;
    std::string name_;

   public:
    Token(int id, std::string name) : id_(id), name_(std::move(name)) {}

    // 供比较策略调用的 key，决定"按什么比较"
    auto compare_key() const { return id_; }

    int id() const { return id_; }
    const std::string& name() const { return name_; }
};

// 定义常用组合的别名
using CopyableEqToken    = Token<Copyable, EqualityComparable>;
using CopyableOrderToken = Token<Copyable, FullyOrdered>;
using UniqueToken        = Token<NonCopyable, EqualityComparable>;
using OpaqueToken        = Token<NonCopyable, NotComparablePolicy>;  // 不可拷贝、不可比较

void demo_copy_compare_policy() {
    std::cout << "=== 第七部分：控制可拷贝性与可比较性 ===" << std::endl;

    // 1) 可拷贝 + 可判等
    std::cout << "\n-- CopyableEqToken --" << std::endl;
    CopyableEqToken a(1, "alpha"), b(1, "beta"), c(2, "gamma");
    auto a_copy = a;  // 拷贝 OK
    std::cout << "  a == b (same id): " << std::boolalpha << (a == b) << std::endl;
    std::cout << "  a == c (diff id): " << (a == c) << std::endl;
    std::cout << "  a_copy == a:      " << (a_copy == a) << std::endl;

    // 2) 可拷贝 + 全序比较
    std::cout << "\n-- CopyableOrderToken --" << std::endl;
    CopyableOrderToken x(10, "x"), y(20, "y"), z(10, "z");
    std::cout << "  x < y:  " << (x < y) << std::endl;
    std::cout << "  x <= z: " << (x <= z) << std::endl;
    std::cout << "  y > x:  " << (y > x) << std::endl;
    std::cout << "  x == z: " << (x == z) << std::endl;

    // 3) 不可拷贝 + 可判等
    std::cout << "\n-- UniqueToken (non-copyable) --" << std::endl;
    UniqueToken u1(100, "unique1"), u2(100, "unique2");
    // auto u3 = u1;  // 编译错误：拷贝已 delete
    std::cout << "  u1 == u2: " << (u1 == u2) << std::endl;
    auto u3 = std::move(u1);  // 移动 OK
    std::cout << "  u3 == u2: " << (u3 == u2) << std::endl;

    // 4) 不可拷贝 + 不可比较
    std::cout << "\n-- OpaqueToken (non-copyable, non-comparable) --" << std::endl;
    OpaqueToken o1(42, "opaque");
    // auto o2 = o1;         // 编译错误：拷贝已 delete
    // bool eq = (o1 == o1); // 编译错误：没有 operator==
    std::cout << "  OpaqueToken created, id=" << o1.id() << std::endl;

    // 编译期验证
    static_assert(std::is_copy_constructible_v<CopyableEqToken>,    "should be copyable");
    static_assert(!std::is_copy_constructible_v<UniqueToken>,        "should NOT be copyable");
    static_assert(!std::is_copy_constructible_v<OpaqueToken>,        "should NOT be copyable");
    static_assert(std::equality_comparable<CopyableEqToken>,         "should be eq-comparable");
    static_assert(std::totally_ordered<CopyableOrderToken>,          "should be fully ordered");
    static_assert(!std::equality_comparable<OpaqueToken>,             "should NOT be comparable");

    std::cout << "\n  All static_assert passed!" << std::endl;
    std::cout << std::endl;
}

// ============================================================================

int main() {
    demo_log_policy();
    demo_multi_policy();
    demo_stateful_policy();
    demo_crtp();
    demo_mixin_policy();
    demo_variadic_policy();
    demo_copy_compare_policy();
    return 0;
}
