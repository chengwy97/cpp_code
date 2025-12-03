#include <iostream>
#include <map>
#include <vector>
#include <type_traits>

int main() {
    int a = 1;
    int& b = a;
    const int c = 2;
    const int& d = c;
    decltype(a) x1 = 0;        // int
    decltype(b) x2 = x1;       // int&
    decltype((b)) x3 = x1;     // int&  (因为(b)是左值表达式)
    decltype(c) x4 = 3;        // const int
    decltype(d) x5 = 4;        // const int&

    std::cout << "type of x1 is int: " << std::is_same<decltype(x1), int>::value << std::endl;
    std::cout << "type of x2 is int&: " << std::is_same<decltype(x2), int&>::value << std::endl;
    std::cout << "type of x3 is int&: " << std::is_same<decltype(x3), int&>::value << std::endl;
    std::cout << "type of x4 is const int: " << std::is_same<decltype(x4), const int>::value << std::endl;
    std::cout << "type of x5 is const int&: " << std::is_same<decltype(x5), const int&>::value << std::endl;

    // int i = {3.14};
    // std::cout << "i: " << i << std::endl;

    struct S {
        int x;
        double y;
    };
    S s = {10, 3.14};
    std::cout << "s.x: " << s.x << ", s.y: " << s.y << std::endl;

    return 0;
}
