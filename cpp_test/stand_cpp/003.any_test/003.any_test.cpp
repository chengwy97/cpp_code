#include <iostream>
#include <variant>
#include <string>
#include <stdexcept>
#include <string_view>

// 默认的 convertFromString 实现（可以特化）
namespace detail {
    template <typename T>
    T convertFromString(const std::string& str) {
        // 对于基本类型，使用标准库转换
        if constexpr (std::is_same_v<T, int>) {
            return std::stoi(str);
        } else if constexpr (std::is_same_v<T, double>) {
            return std::stod(str);
        } else if constexpr (std::is_same_v<T, float>) {
            return std::stof(str);
        } else if constexpr (std::is_same_v<T, long>) {
            return std::stol(str);
        } else if constexpr (std::is_same_v<T, long long>) {
            return std::stoll(str);
        } else if constexpr (std::is_same_v<T, unsigned long>) {
            return std::stoul(str);
        } else if constexpr (std::is_same_v<T, unsigned long long>) {
            return std::stoull(str);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return str;
        } else {
            // 对于自定义类型，需要用户提供特化
            static_assert(std::is_same_v<T, void>,
                "convertFromString<T> must be specialized for custom types");
        }
    }
}

template <typename T>
class AnyType {
public:
    // 使用 std::string 构造
    AnyType(const std::string& str) : value_(str) {}

    // 使用 const char* 构造（转换为 std::string）
    AnyType(const char* str) : value_(std::string(str)) {}

    // 使用 std::string_view 构造（转换为 std::string）
    AnyType(std::string_view str) : value_(std::string(str)) {}

    // 使用类型 T 构造（使用 SFINAE 避免与字符串构造冲突）
    template<typename U = T,
             typename = typename std::enable_if<!std::is_convertible<U, std::string_view>::value>::type>
    AnyType(const U& value) : value_(value) {}

    // 移动构造 - 字符串
    AnyType(std::string&& str) : value_(std::move(str)) {}

    // 移动构造 - 类型 T（使用 SFINAE）
    template<typename U = T,
             typename = typename std::enable_if<!std::is_convertible<std::decay_t<U>, std::string_view>::value>::type>
    AnyType(U&& value) : value_(std::forward<U>(value)) {}

    // 拷贝构造
    AnyType(const AnyType& other) = default;

    // 移动构造
    AnyType(AnyType&& other) = default;

    // 赋值操作符
    AnyType& operator=(const AnyType& other) = default;
    AnyType& operator=(AnyType&& other) = default;

    // 从 std::string 赋值
    AnyType& operator=(const std::string& str) {
        value_ = str;
        return *this;
    }

    // 从 const char* 赋值
    AnyType& operator=(const char* str) {
        value_ = std::string(str);
        return *this;
    }

    // 从类型 T 赋值（使用 SFINAE 避免与字符串赋值冲突）
    template<typename U = T,
             typename = typename std::enable_if<!std::is_convertible<U, std::string_view>::value>::type>
    AnyType& operator=(const U& value) {
        value_ = value;
        return *this;
    }

    // 获取值：如果存储的是 string，则调用 convertFromString 转换
    // 如果存储的是类型 T，则直接返回
    T get() const {
        if (std::holds_alternative<std::string>(value_)) {
            // 存储的是 string，需要转换
            return detail::convertFromString<T>(std::get<std::string>(value_));
        } else {
            // 存储的是类型 T，直接返回
            return std::get<T>(value_);
        }
    }

    // 转换操作符，方便直接使用
    operator T() const {
        return get();
    }

    // 检查当前存储的是 string 还是类型 T
    bool isString() const {
        return std::holds_alternative<std::string>(value_);
    }

    // 获取原始 string（如果存储的是 string）
    const std::string& getString() const {
        if (!isString()) {
            throw std::runtime_error("AnyType does not contain a string");
        }
        return std::get<std::string>(value_);
    }

    // 获取原始值（如果存储的是类型 T）
    const T& getValue() const {
        if (isString()) {
            throw std::runtime_error("AnyType contains a string, not a value");
        }
        return std::get<T>(value_);
    }

private:
    std::variant<std::string, T> value_;
};

// 测试用的自定义类型
struct Position2D {
    double x, y;

    bool operator==(const Position2D& other) const {
        return x == other.x && y == other.y;
    }
};

// 为 Position2D 提供 convertFromString 特化
namespace detail {
    template <>
    Position2D convertFromString<Position2D>(const std::string& str) {
        // 格式: "x;y"
        size_t pos = str.find(';');
        if (pos == std::string::npos) {
            throw std::runtime_error("Invalid Position2D format: " + str);
        }
        Position2D result;
        result.x = std::stod(str.substr(0, pos));
        result.y = std::stod(str.substr(pos + 1));
        return result;
    }
}

int main() {
    // 测试 1: 使用 string 赋值
    AnyType<int> any1("42");
    int val1 = any1;  // 自动转换
    std::cout << "String '42' converted to int: " << val1 << std::endl;

    // 测试 2: 使用类型赋值
    AnyType<int> any2(100);
    int val2 = any2;  // 直接返回
    std::cout << "Int 100 stored and retrieved: " << val2 << std::endl;

    // 测试 3: 自定义类型 - string
    AnyType<Position2D> any3("1.5;2.3");
    Position2D pos1 = any3;
    std::cout << "String '1.5;2.3' converted to Position2D: ("
              << pos1.x << ", " << pos1.y << ")" << std::endl;

    // 测试 4: 自定义类型 - 直接赋值
    Position2D pos2{3.7, 4.9};
    AnyType<Position2D> any4(pos2);
    Position2D pos3 = any4;
    std::cout << "Position2D stored and retrieved: ("
              << pos3.x << ", " << pos3.y << ")" << std::endl;

    // 测试 5: 检查存储类型
    std::cout << "any1 is string: " << any1.isString() << std::endl;
    std::cout << "any2 is string: " << any2.isString() << std::endl;

    // 测试 6: 重新赋值
    any1 = std::string("999");
    std::cout << "any1 reassigned to '999': " << any1.get() << std::endl;
    int val777 = 777;
    any1 = val777;
    std::cout << "any1 reassigned to 777: " << any1.get() << std::endl;
    std::cout << "any1 is string: " << any1.isString() << std::endl;

    return 0;
}
