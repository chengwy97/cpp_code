#include <concepts>
#include <iostream>
#include <type_traits>

template <typename T>
concept HasResult = requires(T t) { t.result; };

template <typename T>
concept HasCode = requires(T t) { t.code; };

template <typename T>
concept HasMessage = requires(T t) {
    { t.message } -> std::same_as<std::string>;
};

template <typename T>
bool isOk(const T& t) {
    if constexpr (std::is_same_v<T, bool>) {
        return t;
    } else if constexpr (std::is_integral_v<T>) {
        return t == 0;
    } else {
        static_assert([]() constexpr { return false; }(), "Unsupported type in isOk");
    }
}

template <typename T>
bool test(const T& t) {
    bool result = false;
    if constexpr (HasResult<T>) {
        result = isOk(t.result);
    } else if constexpr (HasCode<T>) {
        result = isOk(t.code);
    } else {
        static_assert([]() constexpr { return false; }(), "Unsupported type in test");
    }

    if constexpr (HasMessage<T>) {
        std::cout << t.message << std::endl;
    }

    return result;
}

struct Result {
    int         result;
    std::string message;
};

struct Code {
    int         code;
    std::string message;
};

struct OnlyCode {
    int code;
};

struct OnlyResult {
    int result;
};

// struct Message {
//     std::string message;
// };
int main() {
    struct RestoreCoutFlags {
        RestoreCoutFlags() {
            m_cout_flags = std::cout.flags();
            std::cout.setf(std::ios::boolalpha);
        }
        ~RestoreCoutFlags() { std::cout.flags(m_cout_flags); }

       private:
        std::ios::fmtflags m_cout_flags;
    };

    RestoreCoutFlags restore_cout_flags;

    Result result{1, "success"};
    std::cout << test(result) << std::endl;
    Code code{0, "success"};
    std::cout << test(code) << std::endl;
    // Message message{"success"};
    // std::cout << test(message) << std::endl;

    OnlyCode only_code{0};
    std::cout << test(only_code) << std::endl;
    OnlyResult only_result{1};
    std::cout << test(only_result) << std::endl;

    return 0;
}
