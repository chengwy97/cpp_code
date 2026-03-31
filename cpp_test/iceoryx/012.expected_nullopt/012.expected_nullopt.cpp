#include <expected>
#include <iostream>
#include <string>

std::expected<int, std::string> getExpected() {
    return std::expected<int, std::string>(std::in_place, 1);
}

std::expected<int, std::string> getExpectedError() {
    return std::unexpected(std::string("error"));
}

// 注意：iceoryx 的 expected::and_then / or_else 是“回调通知”语义：
// - and_then 要求签名为 void(ValueType&)（或 const 版本）
// - or_else  要求签名为 void(ErrorType&)（或 const 版本）
std::expected<void, std::string> onExpectedValue(const int& value) {
    std::cout << "value: " << value << std::endl;
    return std::expected<void, std::string>(std::in_place);
}

std::expected<void, std::string> onExpectedError(const std::string& error) {
    std::cout << "error: " << error << std::endl;
    return std::unexpected(error);
}

int main() {
    getExpected().and_then(onExpectedValue).or_else(onExpectedError);

    getExpectedError().and_then(onExpectedValue).or_else(onExpectedError);

    return 0;
}
