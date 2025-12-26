#include "version/version.hpp"
#include <iostream>

int main() {
    std::cout << "=== Application Version Information ===" << std::endl;

    // 版本信息
    std::cout << "\n[Version]" << std::endl;
    std::cout << "  Project: " << PROJECT_NAME << std::endl;
    std::cout << "  Version: " << PROJECT_VERSION << std::endl;
    std::cout << "  Version (detailed): " << VERSION_MAJOR << "."
              << VERSION_MINOR << "." << VERSION_PATCH << std::endl;

    // Git 信息
    std::cout << "\n[Git]" << std::endl;
    std::cout << "  Commit ID: " << GIT_COMMIT_ID << std::endl;
    std::cout << "  Commit (short): " << GIT_COMMIT_SHORT << std::endl;
    std::cout << "  Branch: " << GIT_BRANCH << std::endl;
    std::cout << "  Tag: " << GIT_TAG << std::endl;
    std::cout << "  Dirty: " << (GIT_DIRTY ? "Yes" : "No") << std::endl;

    // 构建信息
    std::cout << "\n[Build]" << std::endl;
    std::cout << "  Build time: " << BUILD_TIME << std::endl;
    std::cout << "  Build date: " << BUILD_DATE << std::endl;
    std::cout << "  Build type: " << BUILD_TYPE << std::endl;
    std::cout << "  Build number: " << BUILD_NUMBER << std::endl;
    std::cout << "  Build machine: " << BUILD_MACHINE << std::endl;

    // 系统信息
    std::cout << "\n[System]" << std::endl;
    std::cout << "  OS: " << BUILD_SYSTEM_NAME << " " << BUILD_SYSTEM_VERSION << std::endl;
    std::cout << "  Processor: " << BUILD_SYSTEM_PROCESSOR << std::endl;
    std::cout << "  Hostname: " << HOSTNAME << std::endl;
    std::cout << "  Username: " << USERNAME << std::endl;

    // 编译器信息
    std::cout << "\n[Compiler]" << std::endl;
    std::cout << "  Compiler: " << COMPILER_ID << " " << COMPILER_VERSION << std::endl;
    std::cout << "  C++ Standard: " << CXX_STANDARD << std::endl;
    std::cout << "  CMake version: " << CMAKE_VERSION << std::endl;

    return 0;
}

