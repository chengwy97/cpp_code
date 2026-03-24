#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <memory>

struct Test {
    int    a;
    double b;
    char   c;
};

int main() {
    // 64 字节对齐的存储，避免使用已废弃的 std::aligned_storage
    alignas(64) std::byte storage[sizeof(Test)];
    // std::construct_at 在给定地址上构造对象并开始其生命周期（C++20），无需 C++26 的
    // start_lifetime_as
    Test* p = std::construct_at(reinterpret_cast<Test*>(&storage), 1, 2.0, 'c');
    std::cout << "p->a: " << p->a << std::endl;
    std::cout << "p->b: " << p->b << std::endl;
    std::cout << "p->c: " << p->c << std::endl;
    std::destroy_at(p);
    return 0;
}
