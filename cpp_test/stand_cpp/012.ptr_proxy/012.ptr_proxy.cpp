#include <functional>
#include <iostream>
#include <optional>

class A {
   public:
    A() { std::cout << "A constructor" << std::endl; }
    ~A() { std::cout << "A destructor" << std::endl; }
    void test() { std::cout << "A test" << std::endl; }
    int  test2(int a) {
        std::cout << "A test2" << std::endl;
        return a;
    }
};

template <typename T>
class PtrProxy {
    T& obj_;

   public:
    PtrProxy(T& obj) : obj_(obj) {}
    ~PtrProxy() { std::cout << "PtrProxy destructor" << std::endl; }

    bool beforeInvoke() {
        std::cout << "Before invoke" << std::endl;
        return true;
    }

    void afterInvoke() { std::cout << "After invoke" << std::endl; }

    template <typename F, typename... Args>
    auto operator()(F&& f, Args&&... args)
        -> std::optional<std::conditional_t<std::is_void_v<std::invoke_result_t<F, T*, Args...>>,
                                            bool, std::invoke_result_t<F, T*, Args...>>> {
        using ReturnType = std::invoke_result_t<F, T*, Args...>;
        if (!beforeInvoke()) {
            return std::nullopt;
        }

        if constexpr (std::is_void_v<ReturnType>) {
            std::invoke(std::forward<F>(f), &obj_, std::forward<Args>(args)...);
            afterInvoke();
            return std::optional<bool>(true);
        } else {
            auto result = std::invoke(std::forward<F>(f), &obj_, std::forward<Args>(args)...);
            afterInvoke();
            return std::make_optional<ReturnType>(std::move(result));
        }
    }
};

int main() {
    A           a;
    PtrProxy<A> proxy(a);
    proxy(&A::test);
    auto result = proxy(&A::test2, 1);
    if (result) {
        std::cout << "Result: " << *result << std::endl;
    }
    return 0;
}
