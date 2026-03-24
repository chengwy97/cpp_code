#include <array>
#include <chrono>
#include <iostream>
#include <iox/detail/mpmc_loffli.hpp>
#include <iox/detail/spsc_fifo.hpp>
#include <iox/detail/spsc_sofi.hpp>
#include <thread>
#include <vector>

// ============================================================================
// 1. SpscFifo：严格的单生产者单消费者 FIFO，满了 push 返回 false
// 2. SpscSoFi：带安全溢出的 SPSC 队列，满了 push 会覆盖最旧的元素
// 3. MpmcLoFFLi：多生产者多消费者无锁空闲链表（索引分配器）
// ============================================================================

void demo_spsc_fifo() {
    std::cout << "=== SpscFifo (满了拒绝) ===" << std::endl;

    constexpr uint64_t                       CAPACITY = 5;
    iox::concurrent::SpscFifo<int, CAPACITY> fifo;

    std::jthread producer([&fifo](std::stop_token st) {
        for (int i = 0; !st.stop_requested(); ++i) {
            if (fifo.push(i)) {
                std::cout << "  [producer] pushed: " << i << std::endl;
            } else {
                std::cout << "  [producer] queue full, dropped: " << i << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    std::jthread consumer([&fifo](std::stop_token st) {
        while (!st.stop_requested()) {
            if (auto val = fifo.pop(); val.has_value()) {
                std::cout << "  [consumer] popped: " << val.value() << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(120));
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void demo_spsc_sofi() {
    std::cout << "\n=== SpscSoFi (满了覆盖最旧) ===" << std::endl;

    constexpr uint64_t                       CAPACITY = 5;
    iox::concurrent::SpscSofi<int, CAPACITY> sofi;

    std::jthread producer([&sofi](std::stop_token st) {
        for (int i = 0; !st.stop_requested(); ++i) {
            int  overwritten  = -1;
            bool had_overflow = !sofi.push(i, overwritten);
            if (had_overflow) {
                std::cout << "  [producer] pushed: " << i << " (overwritten oldest: " << overwritten
                          << ")" << std::endl;
            } else {
                std::cout << "  [producer] pushed: " << i << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });

    std::jthread consumer([&sofi](std::stop_token st) {
        while (!st.stop_requested()) {
            int val{};
            if (sofi.pop(val)) {
                std::cout << "  [consumer] popped: " << val << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(800));
}

// ============================================================================
// MpmcLoFFLi — Lock-Free Free List（无锁空闲链表/索引分配器）
//
// 核心概念：管理 [0, capacity) 范围内的索引。
//   pop()  → 从空闲链表中取出一个索引（分配）
//   push() → 将索引归还到空闲链表（释放）
//
// 典型用途：作为无锁对象池的底层索引管理器。
// 多个线程可以同时 pop/push，无需任何锁。
// ============================================================================

struct Sensor {
    int    id{};
    double value{};
};

void demo_mpmc_loffli() {
    std::cout << "\n=== MpmcLoFFLi (无锁索引分配器 → 对象池) ===" << std::endl;

    constexpr uint32_t POOL_SIZE = 8;

    // 1. 准备 LoFFLi 需要的索引内存：至少 (capacity + 1) 个 Index_t
    //    requiredIndexMemorySize 返回字节数，这里作为元素数只会多分配，安全
    iox::concurrent::MpmcLoFFLi loffli;
    iox::concurrent::MpmcLoFFLi::Index_t
        indexMemory[iox::concurrent::MpmcLoFFLi::requiredIndexMemorySize(POOL_SIZE)]{};
    loffli.init(&indexMemory[0], POOL_SIZE);

    // 2. 对象池：LoFFLi 管理索引，索引映射到实际对象
    std::array<Sensor, POOL_SIZE> pool{};

    auto allocate = [&](Sensor** out) -> bool {
        iox::concurrent::MpmcLoFFLi::Index_t idx{};
        if (!loffli.pop(idx)) {
            return false;
        }
        pool[idx] = Sensor{};
        *out      = &pool[idx];
        return true;
    };

    auto deallocate = [&](Sensor* ptr) -> bool {
        auto idx = static_cast<iox::concurrent::MpmcLoFFLi::Index_t>(ptr - pool.data());
        return loffli.push(idx);
    };

    // 3. 多线程并发分配/释放
    constexpr int NUM_THREADS    = 4;
    constexpr int OPS_PER_THREAD = 6;

    auto worker = [&](int thread_id) {
        std::vector<Sensor*> acquired;

        for (int i = 0; i < OPS_PER_THREAD; ++i) {
            Sensor* s = nullptr;
            if (allocate(&s)) {
                s->id    = thread_id * 100 + i;
                s->value = thread_id + i * 0.1;
                std::cout << "  [T" << thread_id << "] alloc slot " << (s - pool.data())
                          << " → Sensor{id=" << s->id << ", value=" << s->value << "}" << std::endl;
                acquired.push_back(s);
            } else {
                std::cout << "  [T" << thread_id << "] pool exhausted, can't alloc" << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // 归还
        for (auto* s : acquired) {
            auto slot = s - pool.data();
            if (deallocate(s)) {
                std::cout << "  [T" << thread_id << "] freed  slot " << slot << std::endl;
            }
        }
    };

    std::vector<std::jthread> threads;
    threads.reserve(NUM_THREADS);
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back(worker, t);
    }
    threads.clear();

    // 4. 验证：所有索引应归还完毕，再次 pop 应能取出 POOL_SIZE 个
    uint32_t                             count = 0;
    iox::concurrent::MpmcLoFFLi::Index_t idx{};
    while (loffli.pop(idx)) {
        ++count;
    }
    std::cout << "  → Pool recovered: " << count << "/" << POOL_SIZE << " slots" << std::endl;
}

int main() {
    // demo_spsc_fifo();
    // demo_spsc_sofi();
    demo_mpmc_loffli();
    return 0;
}
