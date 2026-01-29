#include <frozen/string.h>
#include <frozen/unordered_map.h>

#include <array>
#include <iostream>
#include <magic_enum/magic_enum.hpp>

#include "version/version.hpp"

enum class Language : int {
    日本語 = 10,
    한국어 = 20,
    English = 30,
    😃 = 40,
};

int main() {
    std::cout << magic_enum::enum_name(Language::日本語) << std::endl;   // Japanese
    std::cout << magic_enum::enum_name(Language::한국어) << std::endl;   // Korean
    std::cout << magic_enum::enum_name(Language::English) << std::endl;  // English
    std::cout << magic_enum::enum_name(Language::😃) << std::endl;       // Emoji

    std::cout << std::boolalpha;
    std::cout << (magic_enum::enum_cast<Language>("日本語").value() == Language::日本語)
              << std::endl;  // true

    std::cout << "Git commit ID: " << GIT_COMMIT_ID << std::endl;
    std::cout << "Build time: " << BUILD_TIME << std::endl;
    std::cout << "Build type: " << BUILD_TYPE << std::endl;
    std::cout << "Build system: " << BUILD_SYSTEM << std::endl;
    std::cout << "Build system name: " << BUILD_SYSTEM_NAME << std::endl;
    std::cout << "Build system version: " << BUILD_SYSTEM_VERSION << std::endl;
    std::cout << "Build system processor: " << BUILD_SYSTEM_PROCESSOR << std::endl;
    std::cout << "Build machine: " << BUILD_MACHINE << std::endl;
    std::cout << "Build number: " << BUILD_NUMBER << std::endl;
    std::cout << "Build machine: " << BUILD_MACHINE << std::endl;
    std::cout << "Build number: " << BUILD_NUMBER << std::endl;
    std::cout << "Build machine: " << BUILD_MACHINE << std::endl;
    std::cout << "Build number: " << BUILD_NUMBER << std::endl;

    return 0;
}
