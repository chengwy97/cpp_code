#include <expected>
#include <iostream>
#include <iox/bump_allocator.hpp>
#include <iox/newtype.hpp>
#include <memory>
#include <string>

int main() {
    // 使用 memory 申请内存，然后交给 allocator 使用
    // 先分配一块足够大的原始内存，再将其传递给 BumpAllocator 进行管理

    constexpr std::size_t mem_size  = 1024;
    constexpr std::size_t mem_align = 8;

    // 使用 unique_ptr 管理原始内存，'new std::byte[]' 保证和 bump_allocator 兼容
    auto memory = std::make_unique<std::byte[]>(mem_size);

    // 将原始指针传递给 allocator
    iox::BumpAllocator allocator(memory.get(), mem_size);

    uint64_t count = 1;

    auto allocationResult = allocator.allocate(8, mem_align);
    while (!allocationResult.has_error()) {
        *static_cast<uint64_t*>(allocationResult.value()) = count++;
        allocationResult                                  = allocator.allocate(8, mem_align);
    }

    for (uint64_t i = 0; i < count - 1; i++) {
        std::cout << *(static_cast<uint64_t*>(static_cast<void*>(memory.get())) + i) << std::endl;
    }
    allocator.deallocate();

    return 0;
}
