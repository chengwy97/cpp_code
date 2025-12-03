#include "lib1.h"
#include <iostream>

namespace lib1 {
    int add(int a, int b) {
        return a + b;
    }

    void print_hello() {
        std::cout << "[lib1] Hello from static library 1!" << std::endl;
    }
}

