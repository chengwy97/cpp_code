#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <functional>
#include <iostream>
#include <memory>

// ============================================================================
// 方案一：虚函数实现（运行时多态）
//
// 优点：
//   - 可在运行时替换实现（插件、热更新、配置驱动选择策略）
//   - Mock 天然适配，gmock 就是基于虚函数设计的
//   - 接口清晰，适合跨模块/跨库的边界
//
// 缺点：
//   - 虚表开销：每次调用多一次间接寻址
//   - 必须通过指针/引用持有，不能值语义
//   - 无法内联，编译器优化受限
// ============================================================================

namespace virtual_impl {

template <typename... Args>
class Callable {
   public:
    virtual bool operator()(Args... args) = 0;
    virtual ~Callable()                   = default;
};

template <typename... Args>
class DefaultCallable : public Callable<Args...> {
   public:
    bool operator()(Args... args) override {
        std::cout << "[virtual] DefaultCallable: ";
        ((std::cout << args << " "), ...);
        std::cout << std::endl;
        return true;
    }
};

template <typename... Args>
class CallableHolder {
   public:
    static void setFunc(std::unique_ptr<Callable<Args...>> func) { ptr_ = std::move(func); }
    static Callable<Args...>* getFunc() { return ptr_.get(); }

   private:
    static std::unique_ptr<Callable<Args...>> ptr_;
};

template <typename... Args>
std::unique_ptr<Callable<Args...>> CallableHolder<Args...>::ptr_ =
    std::make_unique<DefaultCallable<Args...>>();

// Mock：继承虚基类
class MockCallable : public Callable<int> {
   public:
    MOCK_METHOD(bool, call, (int));
    bool operator()(int t) override { return call(t); }
};

}  // namespace virtual_impl

// ============================================================================
// 方案二：直接实现（编译期多态 / 无虚函数）
//
// 优点：
//   - 零开销：无虚表，编译器可内联，性能最优
//   - 值语义友好，可以直接存储、拷贝
//   - 编译期检查接口，错误更早暴露
//
// 缺点：
//   - 替换实现需要重新编译（模板参数在编译期确定）
//   - 不适合运行时动态切换策略
//   - Mock 不能直接用 gmock 的 MOCK_METHOD（需要额外适配）
// ============================================================================

namespace direct_impl {

// ---- CRTP 基类：提供公共行为，具体调用逻辑由派生类实现 ----
// Derived 必须实现: bool do_call(Args... args)
template <typename Derived, typename... Args>
class CallableBase {
   public:
    // 对外统一接口：前置 hook → 实际调用 → 后置 hook
    bool operator()(Args... args) {
        before_call();
        bool result = self().do_call(std::forward<Args>(args)...);
        after_call(result);
        return result;
    }

   protected:
    // 可被派生类覆盖的 hook（非虚函数，编译期绑定）
    void before_call() {}
    void after_call(bool /*result*/) {}

   private:
    Derived& self() { return static_cast<Derived&>(*this); }
};

// ---- 默认实现 ----
class DefaultCallable : public CallableBase<DefaultCallable, int> {
   public:
    bool do_call(int t) {
        std::cout << "[crtp] DefaultCallable: " << t << std::endl;
        return true;
    }
};

// ---- 带 hook 的实现：演示 CRTP 的扩展能力 ----
class LoggedCallable : public CallableBase<LoggedCallable, int> {
   public:
    bool do_call(int t) {
        std::cout << "[crtp] LoggedCallable: " << t << std::endl;
        return t > 0;
    }

   protected:
    void before_call() { std::cout << "[crtp]   >> before_call" << std::endl; }
    void after_call(bool result) {
        std::cout << "[crtp]   >> after_call, result=" << std::boolalpha << result << std::endl;
    }
};

// ---- CallableHolder：静态持有，编译期绑定具体 CRTP 派生类 ----
template <typename Func = DefaultCallable>
class CallableHolder {
   public:
    static void  setFunc(Func func) { func_ = std::move(func); }
    static Func& getFunc() { return func_; }
    static bool  invoke(auto&&... args) { return func_(std::forward<decltype(args)>(args)...); }

   private:
    static Func func_;
};

template <typename Func>
Func CallableHolder<Func>::func_{};

// ---- Mock：同样继承 CRTP 基类 ----
class MockCallable : public CallableBase<MockCallable, int> {
   public:
    MOCK_METHOD(bool, do_call, (int));
};

// 也可以用 lambda / std::function，不走 CRTP
using SimpleMock = std::function<bool(int)>;

}  // namespace direct_impl

// ============================================================================
// 测试：虚函数方案
// ============================================================================

TEST(VirtualImpl, DefaultCallable) {
    ASSERT_TRUE((*virtual_impl::CallableHolder<int>::getFunc())(42));
    virtual_impl::CallableHolder<int>::setFunc(nullptr);
}

TEST(VirtualImpl, MockCallable) {
    auto  mock_ptr = std::make_unique<testing::NiceMock<virtual_impl::MockCallable>>();
    auto* mock_raw = mock_ptr.get();

    EXPECT_CALL(*mock_raw, call(42)).WillOnce(testing::Return(true));
    virtual_impl::CallableHolder<int>::setFunc(std::move(mock_ptr));
    ASSERT_TRUE((*virtual_impl::CallableHolder<int>::getFunc())(42));

    virtual_impl::CallableHolder<int>::setFunc(nullptr);
}

// ============================================================================
// 测试：CRTP 方案
// ============================================================================

TEST(CrtpImpl, DefaultCallable) {
    // 默认模板参数就是 DefaultCallable，静态成员已默认构造
    ASSERT_TRUE(direct_impl::CallableHolder<>::invoke(42));
}

TEST(CrtpImpl, LoggedCallable) {
    using Holder = direct_impl::CallableHolder<direct_impl::LoggedCallable>;
    ASSERT_TRUE(Holder::invoke(42));
    ASSERT_FALSE(Holder::invoke(-1));
}

TEST(CrtpImpl, MockWithGmock) {
    // 用 std::function 包装 mock 的引用，避免 reference_wrapper 无法默认构造的问题
    using Holder = direct_impl::CallableHolder<direct_impl::SimpleMock>;

    testing::NiceMock<direct_impl::MockCallable> mock;
    EXPECT_CALL(mock, do_call(42)).WillOnce(testing::Return(true));

    Holder::setFunc([&mock](int t) { return mock(t); });
    ASSERT_TRUE(Holder::invoke(42));
}

TEST(CrtpImpl, MockWithStdFunction) {
    using Holder = direct_impl::CallableHolder<direct_impl::SimpleMock>;

    Holder::setFunc([](int t) { return t == 42; });
    ASSERT_TRUE(Holder::invoke(42));
    ASSERT_FALSE(Holder::invoke(99));
}

// ============================================================================

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
