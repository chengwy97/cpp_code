#include <expected>
#include <iostream>
#include <iox/newtype.hpp>
#include <string>

#include "iox/detail/newtype/arithmetic.hpp"

struct TestType
    : public iox::NewType<TestType, int, iox::newtype::DefaultConstructable,
                          iox::newtype::Convertable, iox::newtype::CopyConstructable,
                          iox::newtype::MoveConstructable, iox::newtype::AssignByValueCopy,
                          iox::newtype::ConstructByValueCopy, iox::newtype::Arithmetic> {
    using ThisType::ThisType;
    using ThisType::operator=;

    const value_type& value() { return iox::newtype::internal::newTypeRefAccessor(*this); }
};

struct TestStdString
    : public iox::NewType<TestStdString, std::string, iox::newtype::DefaultConstructable,
                          iox::newtype::CopyConstructable, iox::newtype::MoveConstructable,
                          iox::newtype::AssignByValueCopy, iox::newtype::ConstructByValueCopy> {
    using ThisType::ThisType;
    using ThisType::operator=;

    const value_type& value() { return iox::newtype::internal::newTypeRefAccessor(*this); }
};

int main() {
    TestType a{1};
    std::cout << a.value() << std::endl;

    TestStdString b{"hello"};
    std::cout << b.value() << std::endl;
    return 0;
}
