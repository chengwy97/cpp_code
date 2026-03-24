// Receiver：连接 Roudi，映射所有段，
//   1. 轮询控制段的 ready 标志
//   2. ready=1 时：解析 (segment_id, offset)，
//      通过 RelativePointer 读取数据段中的 int64 值并打印
//   3. 清零控制段（segment_id=0, offset=0, ready=0）
//
// 用法: receiver

#include <fcntl.h>
#include <sys/mman.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

#include "003.common.hpp"

int main() {
    // ── 1. 从 Roudi 获取段列表 ─────────────────────────────────────────────
    auto segments = getSegmentsFromRoudi();
    std::cout << "[Receiver] Got " << segments.size() << " segments from roudi\n";

    const SegmentInfo* ctrlInfo = findControlSegment(segments);
    if (!ctrlInfo) {
        std::cerr << "[Receiver] No control segment found\n";
        return 1;
    }

    // ── 2. 映射全部段，注册本地首地址 ─────────────────────────────────────
    // 注意：每个进程 mmap 后的虚拟地址与其他进程不同，
    // RelativePointer 用 (id, offset) 在本进程注册表中还原真实指针。
    for (auto& info : segments) {
        // 控制段需要读写（receiver 负责清零）；数据段只读即可
        int prot = info.is_control ? (PROT_READ | PROT_WRITE) : PROT_READ;
        int fd   = ::shm_open(info.shm_name, info.is_control ? O_RDWR : O_RDONLY, 0);
        if (fd < 0) {
            perror("shm_open");
            return 1;
        }

        void* addr = ::mmap(nullptr, info.size, prot, MAP_SHARED, fd, 0);
        ::close(fd);
        if (addr == MAP_FAILED) {
            perror("mmap");
            return 1;
        }

        SegmentRegistry::registerSegment(info.id, addr);
        std::cout << "[Receiver] Mapped segment id=" << info.id << "  name=" << info.shm_name
                  << "  base=" << addr << (info.is_control ? "  [CONTROL]" : "") << "\n";
    }

    // ── 3. 获取控制段中 ControlMessage 的指针 ─────────────────────────────
    void*           ctrlBase  = SegmentRegistry::getBase(ctrlInfo->id);
    ControlMessage* ctrlMsg   = reinterpret_cast<ControlMessage*>(ctrlBase);
    auto            readyAtom = ctrlReady(ctrlMsg);  // std::atomic_ref<uint64_t>

    std::cout << "[Receiver] Polling control segment (id=" << ctrlInfo->id << ")...\n\n";

    // ── 4. 轮询循环 ────────────────────────────────────────────────────────
    while (true) {
        // acquire：确保看到 sender 在置 ready=1 之前对 segment_id/offset 的写入
        if (readyAtom.load(std::memory_order_acquire) == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // ── 4a. 读取控制信息 ────────────────────────────────────────────────
        uint64_t seg_id = ctrlMsg->segment_id;
        uint64_t off    = ctrlMsg->offset;

        std::cout << "[Receiver] Control message arrived: segment_id=" << seg_id
                  << "  offset=" << off << "\n";

        // ── 4b. 通过 RelativePointer 读取数据段中的 int64 ──────────────────
        // 在本进程中，用 SegmentRegistry 解析 (seg_id, off) → 真实地址
        try {
            RelativePointer<int64_t> dataPtr(seg_id,
                                             static_cast<RelativePointer<int64_t>::offset_t>(off));

            // acquire 读：与 sender 的 relaxed 写配合（控制段的 release/acquire 已提供
            // happens-before，所以这里 relaxed 也可见；用 acquire 更保险）
            // 控制段的 release/acquire 已建立 happens-before，data 的 relaxed 写对此处可见；
            // atomic_ref 避免 reinterpret_cast<atomic*> 的对象未构造 UB
            std::atomic_ref<int64_t> dataRef(*dataPtr.get());
            int64_t                  value = dataRef.load(std::memory_order_relaxed);

            std::cout << "[Receiver] Read int64 value=" << value << " from segment id=" << seg_id
                      << " offset=" << off << "\n";
        } catch (const std::exception& e) {
            std::cerr << "[Receiver] Error reading data: " << e.what() << "\n";
        }

        // ── 4c. 清零控制段 ─────────────────────────────────────────────────
        // 先清数据字段，再 release-store 清 ready，
        // 确保 sender 在看到 ready=0 后能安全写入新的 segment_id/offset
        ctrlMsg->segment_id = 0;
        ctrlMsg->offset     = 0;
        readyAtom.store(0, std::memory_order_release);

        std::cout << "[Receiver] Control segment cleared (ready=0)\n\n";
    }

    return 0;
}
