#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>

struct ChunkHeader {
    uint32_t userHeaderSize{0};
    uint32_t userPayloadSize{0};
    uint32_t userPayloadAlignment{1};
    uint32_t userPayloadOffset{0};

    // ===== 核心方法 =====

    void* userPayload() { return reinterpret_cast<uint8_t*>(this) + userPayloadOffset; }

    template <typename T>
    T* userHeader() {
        if (userHeaderSize == 0) {
            return nullptr;
        }
        return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(this) + sizeof(ChunkHeader));
    }

    static ChunkHeader* fromUserPayload(const void* payload) {
        auto ptr = reinterpret_cast<const uint8_t*>(payload);

        // 读取 back-offset（payload 前 4 字节）
        uint32_t offset;
        std::memcpy(&offset, ptr - sizeof(uint32_t), sizeof(uint32_t));

        return (ChunkHeader*)(ptr - offset);
    }
};

namespace {
size_t align(size_t value, size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}
}  // namespace

ChunkHeader* allocateChunk(size_t userPayloadSize, size_t userPayloadAlignment,
                           size_t userHeaderSize) {
    size_t headerSize = sizeof(ChunkHeader);

    // ===== Step 1: 计算 Header + UserHeader 结束地址 =====
    size_t chunkHeaderSize = headerSize + userHeaderSize;

    // ===== Step 2: 计算 back-offset 对齐位置 =====
    size_t backOffsetAlignment = alignof(uint32_t);

    size_t anticipatedBackOffset = align(chunkHeaderSize, backOffsetAlignment);

    // ===== Step 3: payload 起始（未对齐）=====
    size_t unalignedPayload = anticipatedBackOffset + sizeof(uint32_t);

    // ===== Step 4: payload 对齐 =====
    size_t alignedPayload = align(unalignedPayload, userPayloadAlignment);

    // ===== Step 5: 计算总大小 =====
    size_t totalSize = alignedPayload + userPayloadSize;

    // ===== Step 6: 分配 =====
    // 使用 aligned_alloc 以保证分配的首地址满足 userPayloadAlignment 的对齐要求
    // 注意: aligned_alloc 的对齐参数必须是 totalSize 的约数, 否则行为未定义
    // 安全起见, 对齐参数取 userPayloadAlignment 与 alignof(ChunkHeader) 的较大者
    size_t minAlign = std::max(userPayloadAlignment, alignof(ChunkHeader));
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory,cppcoreguidelines-pro-type-cstyle-cast,modernize-use-auto)
    uint8_t* raw =
        reinterpret_cast<uint8_t*>(std::aligned_alloc(minAlign, align(totalSize, minAlign)));

    auto* header = reinterpret_cast<ChunkHeader*>(raw);

    // ===== Step 7: 填 header =====
    header->userHeaderSize       = userHeaderSize;
    header->userPayloadSize      = userPayloadSize;
    header->userPayloadAlignment = userPayloadAlignment;
    header->userPayloadOffset    = alignedPayload;

    // ===== Step 8: 写 back-offset =====
    uint32_t offset = alignedPayload;

    std::memcpy(raw + alignedPayload - sizeof(uint32_t), &offset, sizeof(uint32_t));

    return header;
}

struct MyHeader {
    uint64_t timestamp;
};

struct MyPayload {
    int    x;
    double y;
};

void dumpLayout(const char* label, ChunkHeader* chunk) {
    auto* base = reinterpret_cast<uint8_t*>(chunk);
    std::cout << "[" << label << "] layout:" << std::endl;
    std::cout << "  ChunkHeader     @ offset 0, size " << sizeof(ChunkHeader) << std::endl;
    if (chunk->userHeaderSize > 0) {
        std::cout << "  UserHeader      @ offset " << sizeof(ChunkHeader) << ", size "
                  << chunk->userHeaderSize << std::endl;
    }
    std::cout << "  userPayloadOffset = " << chunk->userPayloadOffset << std::endl;
    std::cout << "  Payload         @ offset " << chunk->userPayloadOffset << ", size "
              << chunk->userPayloadSize << std::endl;

    // 验证 back-offset
    uint32_t backOffset;
    std::memcpy(&backOffset, base + chunk->userPayloadOffset - sizeof(uint32_t), sizeof(uint32_t));
    std::cout << "  back-offset     = " << backOffset << std::endl;
    std::cout << std::endl;
}

void demo_with_user_header() {
    std::cout << "===== 带 UserHeader =====" << std::endl;

    std::unique_ptr<ChunkHeader> chunk(
        allocateChunk(sizeof(MyPayload), alignof(MyPayload), sizeof(MyHeader)));

    auto* payload = static_cast<MyPayload*>(chunk->userPayload());
    payload->x    = 42;
    payload->y    = 3.14;

    auto* uh      = chunk->userHeader<MyHeader>();
    uh->timestamp = 123456;

    dumpLayout("with UserHeader", chunk.get());

    // 从 payload 反推回 ChunkHeader
    auto* recovered = ChunkHeader::fromUserPayload(payload);
    auto* p2        = static_cast<MyPayload*>(recovered->userPayload());
    std::cout << "  recovered: x=" << p2->x << " y=" << p2->y;
    std::cout << " timestamp=" << recovered->userHeader<MyHeader>()->timestamp << std::endl;
    std::cout << std::endl;
}

void demo_without_user_header() {
    std::cout << "===== 不带 UserHeader =====" << std::endl;

    std::unique_ptr<ChunkHeader> chunk(allocateChunk(sizeof(MyPayload), alignof(MyPayload), 0));

    auto* payload = static_cast<MyPayload*>(chunk.get()->userPayload());
    payload->x    = 100;
    payload->y    = 2.718;

    // userHeader 应返回 nullptr
    auto* uh = chunk->userHeader<MyHeader>();
    std::cout << "  userHeader<MyHeader>() = " << uh << " (expected nullptr)" << std::endl;

    dumpLayout("no UserHeader", chunk.get());

    // 从 payload 反推
    auto* recovered = ChunkHeader::fromUserPayload(payload);
    auto* p2        = static_cast<MyPayload*>(recovered->userPayload());
    std::cout << "  recovered: x=" << p2->x << " y=" << p2->y << std::endl;
}

int main() {
    demo_with_user_header();
    demo_without_user_header();
    return 0;
}
