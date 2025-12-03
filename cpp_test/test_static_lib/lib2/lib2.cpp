#include "lib2.h"
#include <iostream>

namespace lib2 {
    int multiply(int a, int b) {
        return a * b;
    }

    void print_world() {
        std::cout << "[lib2] World from static library 2!" << std::endl;
    }
}

