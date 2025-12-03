#include "shared_lib.h"
#include <iostream>

namespace shared_lib {
    int divide(int a, int b) {
        if (b == 0) {
            std::cerr << "[shared_lib] Error: Division by zero!" << std::endl;
            return 0;
        }
        return a / b;
    }

    void print_shared() {
        std::cout << "[shared_lib] Hello from shared library!" << std::endl;
    }
}

