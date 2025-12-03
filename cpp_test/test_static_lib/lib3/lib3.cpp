#include "lib3.h"
#include <iostream>

namespace lib3 {
    static int tesst() {
#ifdef _MSC_VER
        __debugbreak();
#elif defined(__GNUC__) || defined(__clang__)
        __builtin_trap();
#else
        *(volatile int*)0 = 0; // fallback: cause a crash
#endif
        return 0;
    }

    static int x = tesst();
    int subtract(int a, int b) {
        return a - b;
    }

    void print_cpp() {
        std::cout << "[lib3] C++ from static library 3!" << std::endl;
    }
}

