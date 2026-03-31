#include <gsl/assert>
#include <gsl/gsl>
#include <iostream>
#include <iox/static_lifetime_guard.hpp>
#include <thread>

class Foo {
   public:
    Foo() { std::cout << "Foo constructor" << std::endl; }
    ~Foo() { std::cout << "Foo destructor" << std::endl; }
};

class Bar {
   public:
    Bar() : foo_(nullptr) { std::cout << "No Foo Bar constructor" << std::endl; }
    Bar(Foo& foo) : foo_(&foo) {
        std::cout << "Bar constructor" << std::endl;
        (void*)&foo_;
    }
    ~Bar() { std::cout << "Bar destructor" << std::endl; }

   private:
    Foo* foo_;
};

static Foo& fooGuard = iox::StaticLifetimeGuard<Foo>::instance();
static Bar& barGuard =
    iox::StaticLifetimeGuard<Bar>::instance(iox::StaticLifetimeGuard<Foo>::instance());

int main() {
    std::cout << "main" << std::endl;
    auto& fooInstance = iox::StaticLifetimeGuard<Foo>::instance();
    std::cout << "fooInstance: " << &fooInstance << std::endl;
    auto& barInstance = iox::StaticLifetimeGuard<Bar>::instance();
    std::cout << "barInstance: " << &barInstance << std::endl;

    std::cout << "fooGuard: " << &fooGuard << std::endl;
    std::cout << "barGuard: " << &barGuard << std::endl;

    std::cout << "exit main" << std::endl;
    return 0;
}
