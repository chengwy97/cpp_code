/// 演示 iceoryx 无锁并发数据结构的基本用法
/// - MpmcLockFreeQueue          : 多生产者多消费者，固定容量，无锁
/// - MpmcResizeableLockFreeQueue: 多生产者多消费者，容量运行时可变，无锁
/// - MpmcLoFFLi                 : 多生产者多消费者无锁空闲索引池（Free-List）
/// - SpscFifo                   : 单生产者单消费者，固定容量，满则拒绝
/// - SpscSofi                   : 单生产者单消费者，固定容量，满则覆盖最旧

#include <atomic>
#include <cstdint>
#include <iostream>
#include <iox/detail/mpmc_lockfree_queue.hpp>
#include <iox/detail/mpmc_loffli.hpp>
#include <iox/detail/mpmc_resizeable_lockfree_queue.hpp>
#include <iox/detail/spsc_fifo.hpp>
#include <iox/detail/spsc_sofi.hpp>
#include <thread>
#include <vector>

// ──────────────────────────────────────────────────────────────────────────────
// 1. MpmcLockFreeQueue — 多生产者多消费者，编译期固定容量
// ──────────────────────────────────────────────────────────────────────────────
void demo_mpmc_lockfree_queue() {
    std::cout << "\n=== MpmcLockFreeQueue ===\n";

    constexpr uint64_t                                CAPACITY = 4;
    iox::concurrent::MpmcLockFreeQueue<int, CAPACITY> queue;

    std::cout << "capacity=" << queue.capacity() << ", empty=" << queue.empty() << "\n";

    // tryPush: 容量满时返回 false，不覆盖
    for (int i = 0; i < 6; ++i) {
        bool ok = queue.tryPush(i);
        std::cout << "tryPush(" << i << ") -> " << (ok ? "ok" : "full/failed") << "\n";
    }

    std::cout << "size=" << queue.size() << "\n";

    // pop: 空队列返回 iox::nullopt
    while (true) {
        auto val = queue.pop();
        if (!val.has_value())
            break;
        std::cout << "pop -> " << *val << "\n";
    }

    // push（溢出版）：队列满时会弹出最旧元素并返回它
    for (int i = 10; i < 14; ++i) queue.tryPush(i);  // 填满
    auto evicted = queue.push(99);                   // 触发溢出
    if (evicted.has_value()) {
        std::cout << "push(99) evicted -> " << *evicted << "\n";
    }
    while (auto val = queue.pop()) {
        std::cout << "pop -> " << *val << "\n";
    }

    // 多线程：2 个生产者 + 2 个消费者
    iox::concurrent::MpmcLockFreeQueue<int, 128> mt_queue;
    std::atomic<int>                             sum{0};
    constexpr int                                N = 50;

    auto producer = [&](int base) {
        for (int i = 0; i < N; ++i) mt_queue.push(base + i);
    };
    auto consumer = [&]() {
        int local = 0;
        for (int i = 0; i < N; ++i) {
            while (true) {
                auto v = mt_queue.pop();
                if (v.has_value()) {
                    local += *v;
                    break;
                }
            }
        }
        sum.fetch_add(local, std::memory_order_relaxed);
    };

    std::thread p1(producer, 0), p2(producer, N);
    std::thread c1(consumer), c2(consumer);
    p1.join();
    p2.join();
    c1.join();
    c2.join();

    // 0..49 + 50..99 = 4950, 两个消费者各消费 50 个
    int expected = (0 + 99) * 100 / 2;  // = 4950
    std::cout << "MT sum=" << sum.load() << " expected=" << expected
              << (sum.load() == expected ? " [PASS]" : " [FAIL]") << "\n";
}

// ──────────────────────────────────────────────────────────────────────────────
// 2. MpmcResizeableLockFreeQueue — 容量运行时可调
// ──────────────────────────────────────────────────────────────────────────────
void demo_mpmc_resizeable_queue() {
    std::cout << "\n=== MpmcResizeableLockFreeQueue ===\n";

    constexpr uint64_t MAX_CAP = 8;
    // 初始容量 4，上限 8
    iox::concurrent::MpmcResizeableLockFreeQueue<int, MAX_CAP> queue(4);

    std::cout << "maxCapacity=" << queue.maxCapacity() << ", capacity=" << queue.capacity() << "\n";

    for (int i = 0; i < 4; ++i) queue.tryPush(i);
    std::cout << "size after 4 pushes = " << queue.size() << "\n";

    // 扩容到 8，不影响已有元素
    bool ok = queue.setCapacity(8);
    std::cout << "setCapacity(8) -> " << (ok ? "ok" : "fail") << ", capacity=" << queue.capacity()
              << "\n";
    for (int i = 4; i < 8; ++i) queue.tryPush(i);
    std::cout << "size after 8 pushes = " << queue.size() << "\n";

    // 缩容到 3，多余的元素会被丢弃（从最旧开始）
    ok = queue.setCapacity(3, [](int val) { std::cout << "  evicted on resize: " << val << "\n"; });
    std::cout << "setCapacity(3) -> " << (ok ? "ok" : "fail") << ", capacity=" << queue.capacity()
              << ", size=" << queue.size() << "\n";

    while (auto val = queue.pop()) {
        std::cout << "pop -> " << *val << "\n";
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// 3. MpmcLoFFLi — 无锁空闲索引池（Free-List）
//    典型用法：管理对象池的槽位下标，pop 分配下标，push 归还下标
// ──────────────────────────────────────────────────────────────────────────────
void demo_mpmc_loffli() {
    std::cout << "\n=== MpmcLoFFLi (Free-List Index Pool) ===\n";

    constexpr uint32_t POOL_SIZE = 5;
    using Index_t                = iox::concurrent::MpmcLoFFLi::Index_t;

    // 申请外部内存并初始化（运行时容量）
    const uint64_t       mem_size = iox::concurrent::MpmcLoFFLi::requiredIndexMemorySize(POOL_SIZE);
    std::vector<uint8_t> raw(mem_size);
    auto*                indices = reinterpret_cast<Index_t*>(raw.data());

    iox::concurrent::MpmcLoFFLi freeList;
    freeList.init(iox::not_null<Index_t*>(indices), POOL_SIZE);

    // 模拟对象池：分配槽位
    std::vector<int> pool(POOL_SIZE);
    for (int i = 0; i < static_cast<int>(POOL_SIZE); ++i) pool[i] = i * 100;

    Index_t              idx = 0;
    std::vector<Index_t> allocated;
    while (freeList.pop(idx)) {
        std::cout << "alloc slot[" << idx << "] = " << pool[idx] << "\n";
        allocated.push_back(idx);
    }

    // 归还部分槽位
    std::cout << "-- free some slots --\n";
    for (size_t i = 0; i < 3; ++i) {
        freeList.push(allocated[i]);
        std::cout << "free slot[" << allocated[i] << "]\n";
    }

    // 再次分配
    std::cout << "-- re-alloc --\n";
    while (freeList.pop(idx)) {
        std::cout << "re-alloc slot[" << idx << "] = " << pool[idx] << "\n";
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// 4. SpscFifo — 单生产者单消费者，满时拒绝入队
// ──────────────────────────────────────────────────────────────────────────────
void demo_spsc_fifo() {
    std::cout << "\n=== SpscFifo ===\n";

    constexpr uint64_t                       CAPACITY = 4;
    iox::concurrent::SpscFifo<int, CAPACITY> fifo;

    std::cout << "capacity=" << fifo.capacity() << "\n";

    // 填满
    for (int i = 0; i < 6; ++i) {
        bool ok = fifo.push(i);
        std::cout << "push(" << i << ") -> " << (ok ? "ok" : "full") << "\n";
    }

    // 消费
    while (!fifo.empty()) {
        auto val = fifo.pop();
        std::cout << "pop -> " << *val << "\n";
    }

    // 单生产者 + 单消费者线程
    iox::concurrent::SpscFifo<int, 64> mt_fifo;
    constexpr int                      N = 50;
    std::atomic<bool>                  done{false};
    int                                consumer_sum = 0;

    std::thread producer([&]() {
        for (int i = 0; i < N; ++i) {
            while (!mt_fifo.push(i)) { /* 自旋等待空位 */
            }
        }
        done.store(true, std::memory_order_release);
    });

    std::thread consumer([&]() {
        int count = 0;
        while (count < N) {
            auto v = mt_fifo.pop();
            if (v.has_value()) {
                consumer_sum += *v;
                ++count;
            }
        }
    });

    producer.join();
    consumer.join();

    int expected = N * (N - 1) / 2;  // 0+1+...+49 = 1225
    std::cout << "MT sum=" << consumer_sum << " expected=" << expected
              << (consumer_sum == expected ? " [PASS]" : " [FAIL]") << "\n";
}

// ──────────────────────────────────────────────────────────────────────────────
// 5. SpscSofi — 单生产者单消费者，满时覆盖最旧（Safe Overflow FIFO）
//    ValueType 必须 trivially copyable
// ──────────────────────────────────────────────────────────────────────────────
void demo_spsc_sofi() {
    std::cout << "\n=== SpscSofi (Safe Overflow FIFO) ===\n";

    constexpr uint64_t                       CAPACITY = 3;
    iox::concurrent::SpscSofi<int, CAPACITY> sofi;

    std::cout << "capacity=" << sofi.capacity() << "\n";

    // push 超过容量时覆盖最旧，valueOut 接收被挤出的元素
    for (int i = 0; i < 6; ++i) {
        int  evicted    = -1;
        bool overflowed = sofi.push(i, evicted);
        if (overflowed) {
            // push 返回 true 表示发生了溢出（有元素被覆盖）
            std::cout << "push(" << i << ") overflowed, evicted=" << evicted << "\n";
        } else {
            std::cout << "push(" << i << ") ok\n";
        }
    }

    std::cout << "size=" << sofi.size() << "\n";

    int val = -1;
    while (sofi.pop(val)) {
        std::cout << "pop -> " << val << "\n";
    }

    // setCapacity：在无并发情况下动态调整逻辑容量（须 <= CapacityValue）
    iox::concurrent::SpscSofi<int, 8> sofi2;
    sofi2.setCapacity(4);
    std::cout << "after setCapacity(4): capacity=" << sofi2.capacity() << "\n";
    for (int i = 0; i < 6; ++i) {
        int evicted = -1;
        sofi2.push(i, evicted);
    }
    while (sofi2.pop(val)) std::cout << "pop -> " << val << "\n";
}

// ──────────────────────────────────────────────────────────────────────────────

int main() {
    demo_mpmc_lockfree_queue();
    demo_mpmc_resizeable_queue();
    demo_mpmc_loffli();
    demo_spsc_fifo();
    demo_spsc_sofi();
    return 0;
}
