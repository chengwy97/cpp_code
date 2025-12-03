#include <iostream>
#include "lib1.h"
#include "lib2.h"
#include "lib3.h"
#include "shared_lib.h"

int main() {
    std::cout << "=== Testing Static and Shared Libraries ===" << std::endl;

    // 测试静态库 lib1
    lib1::print_hello();
    int sum = lib1::add(10, 20);
    std::cout << "[main] lib1::add(10, 20) = " << sum << std::endl;

    // 测试静态库 lib2
    lib2::print_world();
    int product = lib2::multiply(5, 6);
    std::cout << "[main] lib2::multiply(5, 6) = " << product << std::endl;

    // 测试静态库 lib3
    lib3::print_cpp();
    int difference = lib3::subtract(100, 30);
    std::cout << "[main] lib3::subtract(100, 30) = " << difference << std::endl;

    // 测试动态库 shared_lib
    shared_lib::print_shared();
    int quotient = shared_lib::divide(100, 4);
    std::cout << "[main] shared_lib::divide(100, 4) = " << quotient << std::endl;

    std::cout << "\n=== All libraries linked successfully! ===" << std::endl;

    return 0;
}

