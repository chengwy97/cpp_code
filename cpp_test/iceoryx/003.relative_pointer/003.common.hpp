#pragma once

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// 1. 协议常量 & 数据结构
// ─────────────────────────────────────────────────────────────────────────────

static constexpr const char* ROUDI_SOCK_PATH = "/tmp/cpp_003_roudi.sock";

// 客户端 → Roudi 的请求码
static constexpr uint8_t REQ_GET_SEGMENTS = 0x01;

// Roudi 通过 UDS 发送给客户端的段描述
struct SegmentInfo {
    uint64_t id;             // 段 ID；0 = 控制段，1..N = 数据段
    char     shm_name[64];   // POSIX 共享内存名称
    uint64_t size;           // 段大小（字节）
    uint8_t  is_control;     // 1 = 控制段，0 = 数据段
    uint8_t  _pad[7];
};

// ─────────────────────────────────────────────────────────────────────────────
// 2. 控制段消息布局（放在控制段首部）
//
//   发送流程（sender）：
//     1. 把 int64 值写入某数据段指定偏移
//     2. 填写 segment_id / offset
//     3. ready.store(1, release)   ← 最后设置，是可见性屏障
//
//   接收流程（receiver）：
//     1. 轮询 ready.load(acquire) != 0
//     2. 读取 segment_id / offset
//     3. 通过 RelativePointer 取数据段中的 int64
//     4. 清零：segment_id=0, offset=0, ready.store(0, release)
//
// 注意：segment_id 和 offset 用普通 uint64_t；
//       它们在 ready 写 1 之前写入，在看到 ready=1 之后读取，
//       memory_order_release/acquire 保证可见性。
// ─────────────────────────────────────────────────────────────────────────────

struct ControlMessage {
    // 用 uint64_t 原始内存 + reinterpret_cast 到 std::atomic<uint64_t>
    // 来避免在 mmap 内存上"构造"原子对象的 UB（共享内存全局归零即初值 0）
    uint64_t ready;       // 原子访问，0=空闲，1=有消息
    uint64_t segment_id;  // 目标数据段 ID
    uint64_t offset;      // 数据段内的字节偏移
};

// 对 ready 字段提供原子视图。
// 使用 std::atomic_ref（C++20）而非 reinterpret_cast<std::atomic<uint64_t>*>：
//   - reinterpret_cast 方式：对象从未被构造为 atomic，访问是 UB；
//     同时违反严格别名规则（uint64_t 与 atomic<uint64_t> 非同类型）。
//   - atomic_ref：专为"对已有普通对象施加原子操作"而设计，无 UB，
//     要求对象地址满足 required_alignment（mmap 页对齐，完全满足）。
inline std::atomic_ref<uint64_t> ctrlReady(ControlMessage* msg) {
    return std::atomic_ref<uint64_t>(msg->ready);
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. SegmentRegistry：每个进程独立维护 "段ID → 本地首地址" 映射
// ─────────────────────────────────────────────────────────────────────────────

class SegmentRegistry {
   public:
    static void registerSegment(uint64_t id, void* base) { instance()[id] = base; }

    static void* getBase(uint64_t id) {
        auto it = instance().find(id);
        if (it == instance().end()) {
            throw std::out_of_range("SegmentRegistry: unknown segment id");
        }
        return it->second;
    }

   private:
    static std::unordered_map<uint64_t, void*>& instance() {
        static std::unordered_map<uint64_t, void*> m;
        return m;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 4. RelativePointer：以 (segment_id, offset) 表达跨进程指针
//    不同进程将同一段映射到不同虚拟地址，get() 时各自查本地注册表
// ─────────────────────────────────────────────────────────────────────────────

template <typename T>
class RelativePointer {
   public:
    using offset_t = std::ptrdiff_t;

    RelativePointer() = default;

    RelativePointer(T* ptr, uint64_t segmentId) : m_id(segmentId) {
        auto* base = reinterpret_cast<char*>(SegmentRegistry::getBase(segmentId));
        m_offset   = reinterpret_cast<char*>(ptr) - base;
    }

    // 根据 (id, offset) 直接构造，无需已有真实指针
    RelativePointer(uint64_t segmentId, offset_t offset) : m_id(segmentId), m_offset(offset) {}

    T* get() const {
        auto* base = reinterpret_cast<char*>(SegmentRegistry::getBase(m_id));
        return reinterpret_cast<T*>(base + m_offset);
    }

    T&       operator*() const { return *get(); }
    T*       operator->() const { return get(); }
    uint64_t segmentId() const { return m_id; }
    offset_t offset() const { return m_offset; }

   private:
    uint64_t m_id{0};
    offset_t m_offset{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// 5. 工具函数：连接 Roudi，获取全部段信息
// ─────────────────────────────────────────────────────────────────────────────

inline std::vector<SegmentInfo> getSegmentsFromRoudi() {
    int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); std::exit(1); }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, ROUDI_SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("connect to roudi");
        ::close(fd);
        std::exit(1);
    }

    uint8_t req = REQ_GET_SEGMENTS;
    if (::write(fd, &req, 1) != 1) { perror("write request"); ::close(fd); std::exit(1); }

    uint32_t count = 0;
    if (::read(fd, &count, sizeof(count)) != sizeof(count)) {
        perror("read count"); ::close(fd); std::exit(1);
    }

    std::vector<SegmentInfo> segments(count);
    for (uint32_t i = 0; i < count; ++i) {
        if (::read(fd, &segments[i], sizeof(SegmentInfo)) !=
            static_cast<ssize_t>(sizeof(SegmentInfo))) {
            perror("read SegmentInfo"); ::close(fd); std::exit(1);
        }
    }

    ::close(fd);
    return segments;
}

// 从段列表中找控制段（is_control == 1）
inline const SegmentInfo* findControlSegment(const std::vector<SegmentInfo>& segs) {
    for (const auto& s : segs) {
        if (s.is_control) return &s;
    }
    return nullptr;
}
