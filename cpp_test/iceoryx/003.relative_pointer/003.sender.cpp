// Sender：连接 Roudi，映射所有段，
//   1. 通过 RelativePointer 把 int64 写入指定数据段的指定偏移
//   2. 把 (segment_id, offset) 写入控制段，设置 ready=1 通知 receiver
//
// 用法: sender <data_segment_index(0-based)> <byte_offset> <value>

#include <fcntl.h>
#include <sys/mman.h>

#include <atomic>
#include <cstring>
#include <iostream>

#include "003.common.hpp"

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0]
                  << " <data_seg_index(0-based)> <byte_offset> <int64_value>\n"
                  << "  data_seg_index : index into data segment list (excludes control seg)\n"
                  << "  byte_offset    : byte offset within data segment (must be 8-byte aligned)\n"
                  << "  int64_value    : non-zero int64 value to send\n";
        return 1;
    }

    const uint32_t dataSegIdx = static_cast<uint32_t>(std::stoul(argv[1]));
    const uint64_t offset     = std::stoull(argv[2]);
    const int64_t  value      = std::stoll(argv[3]);

    if (value == 0) {
        std::cerr << "[Sender] value must be non-zero\n";
        return 1;
    }
    if (offset % alignof(int64_t) != 0) {
        std::cerr << "[Sender] offset must be " << alignof(int64_t) << "-byte aligned\n";
        return 1;
    }

    // ── 1. 从 Roudi 获取段列表 ─────────────────────────────────────────────
    auto segments = getSegmentsFromRoudi();
    std::cout << "[Sender] Got " << segments.size() << " segments from roudi\n";

    const SegmentInfo* ctrlInfo = findControlSegment(segments);
    if (!ctrlInfo) {
        std::cerr << "[Sender] No control segment found\n";
        return 1;
    }

    // 收集数据段列表（排除控制段）
    std::vector<const SegmentInfo*> dataSegs;
    for (const auto& s : segments) {
        if (!s.is_control)
            dataSegs.push_back(&s);
    }

    if (dataSegIdx >= dataSegs.size()) {
        std::cerr << "[Sender] data_seg_index " << dataSegIdx << " out of range (max "
                  << dataSegs.size() - 1 << ")\n";
        return 1;
    }

    const SegmentInfo* targetInfo = dataSegs[dataSegIdx];

    if (offset + sizeof(int64_t) > targetInfo->size) {
        std::cerr << "[Sender] offset " << offset << " + 8 exceeds segment size "
                  << targetInfo->size << "\n";
        return 1;
    }

    // ── 2. 映射全部段，注册本地首地址 ─────────────────────────────────────
    for (auto& info : segments) {
        int fd = ::shm_open(info.shm_name, O_RDWR, 0);
        if (fd < 0) {
            perror("shm_open");
            return 1;
        }

        void* addr = ::mmap(nullptr, info.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        ::close(fd);
        if (addr == MAP_FAILED) {
            perror("mmap");
            return 1;
        }

        SegmentRegistry::registerSegment(info.id, addr);
        std::cout << "[Sender] Mapped segment id=" << info.id << "  name=" << info.shm_name
                  << "  base=" << addr << (info.is_control ? "  [CONTROL]" : "") << "\n";
    }

    // ── 3. 用 RelativePointer 写 int64 到目标数据段 ────────────────────────
    //   RelativePointer(segmentId, offset) 直接从 (id, offset) 构造，
    //   get() 在本进程中解析出真实地址
    RelativePointer<int64_t> dataPtr(targetInfo->id,
                                     static_cast<RelativePointer<int64_t>::offset_t>(offset));

    // atomic_ref：对已有 int64_t 存储施加原子写，无需改变原对象类型
    std::atomic_ref<int64_t> dataRef(*dataPtr.get());
    dataRef.store(value, std::memory_order_relaxed);  // 先写数据（relaxed）

    std::cout << "[Sender] Wrote int64 value=" << value << " to segment id=" << targetInfo->id
              << " (" << targetInfo->shm_name << ") offset=" << offset << "\n";

    // ── 4. 写控制段，通知 receiver ─────────────────────────────────────────
    void*           ctrlBase = SegmentRegistry::getBase(ctrlInfo->id);
    ControlMessage* ctrlMsg  = reinterpret_cast<ControlMessage*>(ctrlBase);

    // 确保控制段当前空闲（ready == 0），否则等待
    auto     readyRef = ctrlReady(ctrlMsg);  // std::atomic_ref<uint64_t>，轻量值语义
    uint64_t expected = 0;
    while (!readyRef.compare_exchange_weak(expected, 0, std::memory_order_relaxed,
                                           std::memory_order_relaxed)) {
        expected = 0;
        std::cout << "[Sender] Control segment busy, waiting...\n";
        for (volatile int i = 0; i < 100000; ++i) {
        }
    }

    // 先填写 segment_id 和 offset，再置 ready=1（release 保证顺序）
    ctrlMsg->segment_id = targetInfo->id;
    ctrlMsg->offset     = offset;
    readyRef.store(1, std::memory_order_release);

    std::cout << "[Sender] Control message sent: segment_id=" << ctrlMsg->segment_id
              << "  offset=" << ctrlMsg->offset << "  ready=1\n";
    return 0;
}
