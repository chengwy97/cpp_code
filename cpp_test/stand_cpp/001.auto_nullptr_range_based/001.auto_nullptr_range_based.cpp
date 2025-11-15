#include <iostream>
#include <map>
#include <vector>

void test(int p) {
    std::cout << "int test(int p)" << std::endl;
}

void test(char* p) {
    std::cout << "char* test(char* p)" << std::endl;
}

int main() {
    auto n = 10;  // int
    std::cout << typeid(n).name() << std::endl;
    auto pi = 3.1415;  // double
    std::cout << typeid(pi).name() << std::endl;
    std::string s = "hi";
    std::cout << typeid(s).name() << std::endl;
    int* p = nullptr;  // 安全空指针
    std::cout << typeid(p).name() << std::endl;

    // int test = p;
    [[maybe_unused]] int test2 = NULL;

    test(nullptr);
    // test(NULL);

    std::vector<int> v{1, 2, 3, 4};
    for (auto& x : v) {
        x *= 2;
    }
    for (auto x : v) std::cout << x << " ";
    std::cout << "\n";

    std::map<std::string, int> m{{"a", 1}, {"b", 2}};
    for (auto it = m.begin(); it != m.end(); ++it)
        std::cout << it->first << ":" << it->second << " ";
    std::cout << "\n";

    return 0;
}
