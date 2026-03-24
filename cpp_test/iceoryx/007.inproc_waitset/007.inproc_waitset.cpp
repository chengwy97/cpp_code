// 单进程内 WaitSet 等价实现示例（无需 RouDi）
//
// 核心思路：WaitSet 本质是 "共享信号量 + 每个事件源一个原子标志"
//   - 任意事件源触发时：set flag → post semaphore（唤醒等待方）
//   - 等待方：sem.wait() → 扫描所有事件源的 flag → 收集并清除已触发源
//
// 本文件演示：
//   demo1 - 单事件源：UnnamedSemaphore 最基础用法
//   demo2 - 多事件源：自制 MiniWaitSet，语义与 iox::popo::WaitSet 一致
//   demo3 - 定时等待：timedWait + SemaphoreWaitState::TIMEOUT
//   demo4 - 回调风格（Listener 等价）：后台线程循环等待，主线程发回调
//
// 编译依赖：iceoryx_hoofs  （不需要 iceoryx_posh，不需要 RouDi）

#include <atomic>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "iox/duration.hpp"
#include "iox/optional.hpp"
#include "iox/unnamed_semaphore.hpp"

// ─── 工具：原地初始化 UnnamedSemaphore ──────────────────────────────────────
// UnnamedSemaphore 不可移动，必须直接在 optional 中原地构造（通过 Builder::create）

static void initSemaphore(iox::optional<iox::UnnamedSemaphore>& sem, uint32_t init = 0) {
    auto result = iox::UnnamedSemaphoreBuilder()
                      .initialValue(init)
                      .isInterProcessCapable(false)  // 进程内，无需跨进程共享内存
                      .create(sem);
    if (result.has_error()) {
        std::cerr << "[ERROR] create semaphore failed\n";
    }
}

// ─── demo1：单事件源 ─────────────────────────────────────────────────────────

void demo1_single_source() {
    std::cout << "\n=== demo1: single source (UnnamedSemaphore) ===\n";

    iox::optional<iox::UnnamedSemaphore> sem;
    initSemaphore(sem, 0);

    // 生产者线程：50ms 后 post
    std::thread producer([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::cout << "  [producer] posting\n";
        sem->post().or_else([](auto) { std::cerr << "post failed\n"; });
    });

    std::cout << "  [consumer] waiting...\n";
    sem->wait().or_else([](auto) { std::cerr << "wait failed\n"; });
    std::cout << "  [consumer] woke up\n";

    producer.join();
}

// ─── MiniWaitSet：多事件源 WaitSet 等价物 ──────────────────────────────────

struct EventSource {
    std::string       name;
    std::atomic<bool> triggered{false};
};

// 触发一个事件源，并唤醒 WaitSet（由调用者传入信号量引用）
inline void fireEvent(EventSource& src, iox::optional<iox::UnnamedSemaphore>& wakeSem) {
    src.triggered.store(true, std::memory_order_release);
    wakeSem->post().or_else([](auto) { std::cerr << "post failed\n"; });
}

class MiniWaitSet {
   public:
    MiniWaitSet() { initSemaphore(m_sem, 0); }

    void attach(EventSource& src) {
        std::lock_guard<std::mutex> lk(m_mtx);
        m_sources.push_back(&src);
    }

    // 阻塞等待，返回所有已触发的事件源（自动清除 flag）
    std::vector<EventSource*> wait() {
        m_sem->wait().or_else([](auto) { std::cerr << "wait failed\n"; });
        return collectTriggered();
    }

    // 超时等待，timeout 期间无事件则返回空 vector
    std::vector<EventSource*> timedWait(iox::units::Duration timeout) {
        auto state = m_sem->timedWait(timeout);
        if (!state.has_error() && state.value() == iox::SemaphoreWaitState::TIMEOUT) {
            return {};
        }
        return collectTriggered();
    }

    // 供 EventSource 触发时调用
    iox::optional<iox::UnnamedSemaphore>& semaphore() { return m_sem; }

   private:
    std::vector<EventSource*> collectTriggered() {
        std::vector<EventSource*>   result;
        std::lock_guard<std::mutex> lk(m_mtx);
        for (auto* src : m_sources) {
            // compare_exchange 保证 flag 只被"消费"一次
            bool expected = true;
            if (src->triggered.compare_exchange_strong(expected, false,
                                                       std::memory_order_acq_rel)) {
                result.push_back(src);
            }
        }
        return result;
    }

    iox::optional<iox::UnnamedSemaphore> m_sem;
    std::mutex                           m_mtx;
    std::vector<EventSource*>            m_sources;
};

// ─── demo2：多事件源 ─────────────────────────────────────────────────────────

void demo2_multi_source() {
    std::cout << "\n=== demo2: multi source (MiniWaitSet) ===\n";

    MiniWaitSet ws;
    EventSource srcA{"A"}, srcB{"B"}, srcC{"C"};
    ws.attach(srcA);
    ws.attach(srcB);
    ws.attach(srcC);

    // 生产者：依次触发 B → A → C
    std::thread producer([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        std::cout << "  [fire] B\n";
        fireEvent(srcB, ws.semaphore());

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::cout << "  [fire] A\n";
        fireEvent(srcA, ws.semaphore());

        // C 在消费者第二轮 wait 之前触发
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        std::cout << "  [fire] C\n";
        fireEvent(srcC, ws.semaphore());
    });

    // 消费者：等两轮
    for (int round = 0; round < 2; ++round) {
        auto triggered = ws.wait();
        std::cout << "  [round " << round << "] triggered:";
        for (auto* src : triggered) {
            std::cout << " " << src->name;
        }
        std::cout << "\n";
    }

    producer.join();
}

// ─── demo3：定时等待 ─────────────────────────────────────────────────────────

void demo3_timed_wait() {
    std::cout << "\n=== demo3: timed wait ===\n";

    MiniWaitSet ws;
    EventSource src{"X"};
    ws.attach(src);

    // 第一轮：超时（无事件）
    std::cout << "  [timedWait 50ms] expecting TIMEOUT...\n";
    auto result = ws.timedWait(iox::units::Duration::fromMilliseconds(50));
    std::cout << "  result: " << (result.empty() ? "TIMEOUT ✓" : "triggered (unexpected)") << "\n";

    // 第二轮：20ms 后事件来临，100ms 超时
    std::thread t([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        fireEvent(src, ws.semaphore());
    });

    std::cout << "  [timedWait 100ms] expecting event...\n";
    result = ws.timedWait(iox::units::Duration::fromMilliseconds(100));
    std::cout << "  result: " << (result.empty() ? "TIMEOUT (unexpected)" : "got X ✓") << "\n";
    t.join();
}

// ─── demo4：回调风格（Listener 等价）────────────────────────────────────────

// Listener 等价：后台线程持续等待，事件到来时调用注册的回调
class MiniListener {
   public:
    using Callback = std::function<void(EventSource&)>;

    void attachWithCallback(EventSource& src, Callback cb) {
        std::lock_guard<std::mutex> lk(m_mtx);
        m_entries.push_back({&src, std::move(cb)});
        m_ws.attach(src);
    }

    void start() {
        m_running.store(true, std::memory_order_release);
        m_thread = std::thread([this] { loop(); });
    }

    void stop() {
        m_running.store(false, std::memory_order_release);
        // 用一个哨兵事件唤醒 timedWait，避免无限阻塞
        m_ws.semaphore()->post().or_else([](auto) {});
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    // 供外部触发事件源
    iox::optional<iox::UnnamedSemaphore>& semaphore() { return m_ws.semaphore(); }

   private:
    void loop() {
        while (m_running.load(std::memory_order_acquire)) {
            auto triggered = m_ws.timedWait(iox::units::Duration::fromMilliseconds(200));
            std::lock_guard<std::mutex> lk(m_mtx);
            for (auto* src : triggered) {
                for (auto& entry : m_entries) {
                    if (entry.src == src) {
                        entry.cb(*src);
                        break;
                    }
                }
            }
        }
    }

    struct Entry {
        EventSource* src;
        Callback     cb;
    };

    MiniWaitSet        m_ws;
    std::vector<Entry> m_entries;
    std::mutex         m_mtx;
    std::atomic<bool>  m_running{false};
    std::thread        m_thread;
};

void demo4_listener_style() {
    std::cout << "\n=== demo4: listener / callback style ===\n";

    MiniListener listener;
    EventSource  btn{"Button"};
    EventSource  timer{"Timer"};

    listener.attachWithCallback(
        btn, [](EventSource& s) { std::cout << "  [callback] " << s.name << " pressed!\n"; });
    listener.attachWithCallback(
        timer, [](EventSource& s) { std::cout << "  [callback] " << s.name << " expired!\n"; });

    listener.start();

    // 模拟事件触发
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    fireEvent(btn, listener.semaphore());

    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    fireEvent(timer, listener.semaphore());

    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    fireEvent(btn, listener.semaphore());

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    listener.stop();
    std::cout << "  listener stopped\n";
}

// ─── main ────────────────────────────────────────────────────────────────────

int main() {
    demo1_single_source();
    demo2_multi_source();
    demo3_timed_wait();
    demo4_listener_style();
    return 0;
}
