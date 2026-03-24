// UserTrigger 使用示例
//
// 覆盖内容：
//   ① WaitSet + 单 UserTrigger（带 callback）：阻塞等待，子线程触发
//   ② WaitSet + 多 UserTrigger（按 notificationId / doesOriginateFrom 分发）
//   ③ WaitSet::timedWait（超时无触发 vs. 超时内触发）
//   ④ Listener + UserTrigger（异步 callback，自动后台线程）
//   ⑤ hasTriggered() 状态管理
//   ⑥ createNotificationCallback 带上下文数据（ContextData）
//
// 运行前提：需要 iox-roudi 守护进程已在运行
//   $ iox-roudi &
//   $ ./cpp_test_iceoryx_006.user_trigger

#include <atomic>
#include <iostream>
#include <thread>

#include "iceoryx_posh/popo/listener.hpp"
#include "iceoryx_posh/popo/notification_callback.hpp"
#include "iceoryx_posh/popo/user_trigger.hpp"
#include "iceoryx_posh/popo/wait_set.hpp"
#include "iceoryx_posh/runtime/posh_runtime.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// 全局 callback 函数（createNotificationCallback 只接受普通函数指针，不接受捕获 lambda）
// ─────────────────────────────────────────────────────────────────────────────

static void onCyclicTick(iox::popo::UserTrigger* /*trigger*/) {
    std::cout << "  [callback] cyclicTick fired\n";
}

static void onAlarmA(iox::popo::UserTrigger* /*trigger*/) {
    std::cout << "  [callback] alarmA fired\n";
}

static void onAlarmB(iox::popo::UserTrigger* /*trigger*/) {
    std::cout << "  [callback] alarmB fired\n";
}

// callback 带上下文数据：第二个参数为指向用户自定义数据的指针
struct HeartbeatCtx {
    int  beatCount{0};
    bool verbose{true};
};

static void onHeartbeat(iox::popo::UserTrigger* /*trigger*/, HeartbeatCtx* ctx) {
    ++ctx->beatCount;
    if (ctx->verbose) {
        std::cout << "  [listener callback] heartbeat #" << ctx->beatCount << "\n";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// DEMO 1  WaitSet + 单 UserTrigger + callback
//
// 流程：
//   主线程  attach → waitset.wait() 阻塞
//   子线程  每 300 ms trigger() 一次，共 3 次，然后结束
//   主线程  收到通知 → 调用 (*notification)() → onCyclicTick
// ─────────────────────────────────────────────────────────────────────────────

static void demo_waitset_single() {
    std::cout << "\n=== DEMO 1: WaitSet + 单 UserTrigger ===\n";

    iox::popo::WaitSet<>   waitset;
    iox::popo::UserTrigger trigger;

    // 附加：notificationId=0，带 callback
    waitset.attachEvent(trigger, 0U, iox::popo::createNotificationCallback(onCyclicTick))
        .or_else([](auto& e) {
            std::cerr << "attachEvent failed: " << static_cast<int>(e) << "\n";
            std::exit(1);
        });

    std::atomic<bool> done{false};
    std::thread       worker([&] {
        for (int i = 0; i < 3; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            std::cout << "  [worker] trigger #" << i + 1 << "\n";
            trigger.trigger();
        }
        done = true;
    });

    int received = 0;
    while (!done || received < 3) {
        auto notifications = waitset.wait();
        for (auto& n : notifications) {
            (*n)();  // 调用已附加的 callback（onCyclicTick）
            ++received;
        }
    }

    worker.join();
    waitset.detachEvent(trigger);
    std::cout << "  共收到 " << received << " 次通知 ✓\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// DEMO 2  WaitSet + 多 UserTrigger（按 notificationId 或 doesOriginateFrom 分发）
//
// 两种分发方式：
//   a. 检查 notification->getNotificationId()（编译期约定 ID）
//   b. 检查 notification->doesOriginateFrom(&triggerX)（运行期判断来源）
// ─────────────────────────────────────────────────────────────────────────────

static void demo_waitset_multi() {
    std::cout << "\n=== DEMO 2: WaitSet + 多 UserTrigger (多路复用) ===\n";

    iox::popo::WaitSet<>   waitset;
    iox::popo::UserTrigger triggerA;
    iox::popo::UserTrigger triggerB;

    constexpr uint64_t ID_A = 10U;
    constexpr uint64_t ID_B = 20U;

    // 方式 a：附加时指定不同 ID，收到通知后按 ID 分发
    waitset.attachEvent(triggerA, ID_A, iox::popo::createNotificationCallback(onAlarmA))
        .or_else([](auto&) { std::exit(1); });
    waitset.attachEvent(triggerB, ID_B, iox::popo::createNotificationCallback(onAlarmB))
        .or_else([](auto&) { std::exit(1); });

    std::thread worker([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        triggerA.trigger();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        triggerB.trigger();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        triggerA.trigger();
    });

    int total = 0;
    while (total < 3) {
        auto notifications = waitset.wait();
        for (auto& n : notifications) {
            auto id = n->getNotificationId();

            // 方式 a：按 ID 分发
            std::cout << "  [waitset] notificationId=" << id;
            if (id == ID_A)
                std::cout << " → A";
            else if (id == ID_B)
                std::cout << " → B";
            std::cout << "\n";

            // 方式 b：按来源对象分发
            if (n->doesOriginateFrom(&triggerA)) {
                std::cout << "  [waitset] doesOriginateFrom(&triggerA) = true\n";
            } else if (n->doesOriginateFrom(&triggerB)) {
                std::cout << "  [waitset] doesOriginateFrom(&triggerB) = true\n";
            }

            (*n)();  // 调用对应 callback
            ++total;
        }
    }

    worker.join();
    waitset.detachEvent(triggerA);
    waitset.detachEvent(triggerB);
    std::cout << "  多路复用分发 ✓\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// DEMO 3  WaitSet::timedWait（超时处理）
// ─────────────────────────────────────────────────────────────────────────────

static void demo_timed_wait() {
    std::cout << "\n=== DEMO 3: timedWait 超时 ===\n";

    iox::popo::WaitSet<>   waitset;
    iox::popo::UserTrigger trigger;
    waitset.attachEvent(trigger, 0U, iox::popo::createNotificationCallback(onCyclicTick))
        .or_else([](auto&) { std::exit(1); });

    // ── ① 无触发 → 超时后返回空 vector ─────────────────────────────────
    {
        auto notifications = waitset.timedWait(iox::units::Duration::fromMilliseconds(100));
        std::cout << "  timedWait(100ms) 无触发 → notifications.size()=" << notifications.size()
                  << (notifications.empty() ? "  (超时) ✓" : "") << "\n";
    }

    // ── ② 有触发 → 超时内返回 ───────────────────────────────────────────
    {
        std::thread([&] {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            trigger.trigger();
        }).detach();

        auto notifications = waitset.timedWait(iox::units::Duration::fromMilliseconds(500));
        std::cout << "  timedWait(500ms) 有触发 → notifications.size()=" << notifications.size()
                  << (notifications.size() == 1 ? "  ✓" : "") << "\n";
        for (auto& n : notifications) (*n)();
    }

    waitset.detachEvent(trigger);
}

// ─────────────────────────────────────────────────────────────────────────────
// DEMO 4  Listener + UserTrigger（异步 callback，自动后台线程）
//
// Listener 自带后台线程；attach 后 callback 由该线程异步调用，
// 主线程不需要轮询。
// ─────────────────────────────────────────────────────────────────────────────

static void demo_listener() {
    std::cout << "\n=== DEMO 4: Listener + UserTrigger (异步 callback) ===\n";

    iox::popo::Listener    listener;
    iox::popo::UserTrigger heartbeat;

    HeartbeatCtx ctx{0, true};

    // createNotificationCallback 带 ContextData：第二个参数为上下文引用
    listener.attachEvent(heartbeat, iox::popo::createNotificationCallback(onHeartbeat, ctx))
        .or_else([](auto& e) {
            std::cerr << "Listener attachEvent failed: " << static_cast<int>(e) << "\n";
            std::exit(1);
        });

    // 主线程每 200 ms 触发一次，共 4 次
    for (int i = 0; i < 4; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        heartbeat.trigger();
    }

    // 等待 Listener 后台线程处理完所有 callback
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    listener.detachEvent(heartbeat);
    std::cout << "  Listener 共处理 " << ctx.beatCount << " 次 heartbeat ✓\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// DEMO 5  hasTriggered() 状态管理
//
// trigger()  → hasTriggered() == true
// WaitSet 处理后（收到通知）→ hasTriggered() 被重置为 false
// ─────────────────────────────────────────────────────────────────────────────

static void demo_has_triggered() {
    std::cout << "\n=== DEMO 5: hasTriggered() 状态 ===\n";

    iox::popo::WaitSet<>   waitset;
    iox::popo::UserTrigger trigger;
    waitset.attachEvent(trigger, 0U, iox::popo::createNotificationCallback(onCyclicTick))
        .or_else([](auto&) { std::exit(1); });

    std::cout << "  触发前: hasTriggered()=" << trigger.hasTriggered() << "\n";  // 0

    trigger.trigger();
    std::cout << "  trigger() 后: hasTriggered()=" << trigger.hasTriggered() << "\n";  // 1

    // WaitSet 消费通知后会重置状态
    auto notifications = waitset.timedWait(iox::units::Duration::fromMilliseconds(200));
    std::cout << "  timedWait 返回 " << notifications.size() << " 条通知\n";

    std::cout << "  timedWait 后: hasTriggered()=" << trigger.hasTriggered() << "\n";  // 0

    waitset.detachEvent(trigger);
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main() {
    // WaitSet / Listener 需要 RouDi 守护进程和 PoshRuntime 初始化
    iox::runtime::PoshRuntime::initRuntime("iox-cpp-user-trigger-demo");

    demo_waitset_single();
    demo_waitset_multi();
    demo_timed_wait();
    demo_listener();
    demo_has_triggered();

    std::cout << "\n=== 全部 Demo 完成 ===\n";
    return 0;
}
