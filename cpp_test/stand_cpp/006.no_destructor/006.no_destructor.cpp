#include <atomic>
#include <barrier>
#include <cassert>
#include <future>
#include <iostream>
#include <latch>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <variant>
#include <vector>

class Test {
   public:
    Test(int a, double b, std::string c) : a_(a), b_(b), c_(std::move(c)) {
        std::cout << "Test constructor" << std::endl;
    }
    ~Test() { std::cout << "Test destructor" << std::endl; }

   private:
    int         a_;
    double      b_;
    std::string c_;
};

template <typename T>
class NoDestructor {
   public:
    template <typename... Args>
    explicit NoDestructor(Args&&... args)
        requires std::constructible_from<T, Args...> &&
                 (sizeof(NoDestructor<T>::ptr_) >= sizeof(T)) &&
                 (alignof(T) <= alignof(NoDestructor<T>::ptr_))
    {
        std::cout << "NoDestructor constructor: type -> " << typeid(T).name() << std::endl;

        new (ptr_) T(std::forward<Args>(args)...);
    }
    ~NoDestructor() { std::cout << "NoDestructor destructor" << std::endl; }

   private:
    alignas(T) char ptr_[sizeof(T)];
};

NoDestructor<Test> no_destructor(1, 2.0, "test");

NoDestructor<int> no_destructor_int(1);

int main(int argc, char* argv[]) {
    std::cout << "test" << std::endl;
    return 0;
}
