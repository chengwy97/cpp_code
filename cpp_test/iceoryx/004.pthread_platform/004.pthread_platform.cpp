// iceoryx_platform/pthread.hpp 接口使用示例
//
// 覆盖接口：
//   线程  : iox_pthread_create / join / self / setname_np / getname_np
//   互斥属性: iox_pthread_mutexattr_init/destroy/setpshared/settype/setprotocol/setrobust
//   互斥操作: iox_pthread_mutex_init/destroy/lock/trylock/unlock/consistent

#include <atomic>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

#include "iceoryx_platform/pthread.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// 工具宏：检查 pthread 返回值
// ─────────────────────────────────────────────────────────────────────────────
#define CHECK(expr)                                                                  \
    do {                                                                             \
        int _rc = (expr);                                                            \
        if (_rc != 0) {                                                              \
            std::cerr << "[FAIL] " #expr " -> " << _rc << " (" << std::strerror(_rc) \
                      << ") at " __FILE__ ":" << __LINE__ << "\n";                   \
            std::exit(1);                                                            \
        }                                                                            \
    } while (0)

// ─────────────────────────────────────────────────────────────────────────────
// DEMO 1  线程创建、命名、获名、join、self
// ─────────────────────────────────────────────────────────────────────────────

struct ThreadArg {
    std::string name;
    int         id;
};

static void* threadFunc(void* arg) {
    auto* a = static_cast<ThreadArg*>(arg);

    // 设置本线程名称（Linux 限 15 字符）
    CHECK(iox_pthread_setname_np(iox_pthread_self(), a->name.c_str()));

    // 读回名称验证
    char buf[32]{};
    CHECK(iox_pthread_getname_np(iox_pthread_self(), buf, sizeof(buf)));

    std::cout << "  [thread " << a->id << "] self=" << iox_pthread_self() << "  name=\"" << buf
              << "\"\n";
    return nullptr;
}

static void demo_thread() {
    std::cout << "\n=== DEMO 1: 线程创建 / 命名 / join ===\n";

    constexpr int N = 3;
    iox_pthread_t threads[N];
    ThreadArg     args[N];

    for (int i = 0; i < N; ++i) {
        args[i] = {"worker-" + std::to_string(i), i};
        CHECK(iox_pthread_create(&threads[i], nullptr, threadFunc, &args[i]));
    }
    for (int i = 0; i < N; ++i) {
        CHECK(iox_pthread_join(threads[i], nullptr));
    }

    std::cout << "  主线程 self=" << iox_pthread_self() << "\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// DEMO 2  普通互斥锁（NORMAL + PROCESS_PRIVATE）：多线程安全计数器
// ─────────────────────────────────────────────────────────────────────────────

struct CounterArg {
    iox_pthread_mutex_t* mtx;
    long*                counter;
    int                  iters;
};

static void* counterFunc(void* arg) {
    auto* a = static_cast<CounterArg*>(arg);
    for (int i = 0; i < a->iters; ++i) {
        CHECK(iox_pthread_mutex_lock(a->mtx));
        ++(*a->counter);
        CHECK(iox_pthread_mutex_unlock(a->mtx));
    }
    return nullptr;
}

static void demo_normal_mutex() {
    std::cout << "\n=== DEMO 2: NORMAL 互斥锁 —— 多线程计数器 ===\n";

    // 构造属性：PROCESS_PRIVATE + NORMAL
    iox_pthread_mutexattr_t attr;
    CHECK(iox_pthread_mutexattr_init(&attr));
    CHECK(iox_pthread_mutexattr_setpshared(&attr, IOX_PTHREAD_PROCESS_PRIVATE));
    CHECK(iox_pthread_mutexattr_settype(&attr, IOX_PTHREAD_MUTEX_NORMAL));

    iox_pthread_mutex_t mtx;
    CHECK(iox_pthread_mutex_init(&mtx, &attr));
    CHECK(iox_pthread_mutexattr_destroy(&attr));

    constexpr int N       = 4;
    constexpr int ITERS   = 100000;
    long          counter = 0;

    iox_pthread_t threads[N];
    CounterArg    args[N];
    for (int i = 0; i < N; ++i) {
        args[i] = {&mtx, &counter, ITERS};
        CHECK(iox_pthread_create(&threads[i], nullptr, counterFunc, &args[i]));
    }
    for (int i = 0; i < N; ++i) CHECK(iox_pthread_join(threads[i], nullptr));

    CHECK(iox_pthread_mutex_destroy(&mtx));

    std::cout << "  期望=" << (long)N * ITERS << "  实际=" << counter
              << (counter == (long)N * ITERS ? "  ✓ 一致" : "  ✗ 数据竞争！") << "\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// DEMO 3  递归互斥锁（RECURSIVE）：同一线程可重复加锁
// ─────────────────────────────────────────────────────────────────────────────

static void recursiveLock(iox_pthread_mutex_t* mtx, int depth) {
    if (depth == 0)
        return;
    CHECK(iox_pthread_mutex_lock(mtx));
    std::cout << "  locked depth=" << depth << "\n";
    recursiveLock(mtx, depth - 1);
    CHECK(iox_pthread_mutex_unlock(mtx));
    std::cout << "  unlocked depth=" << depth << "\n";
}

static void demo_recursive_mutex() {
    std::cout << "\n=== DEMO 3: RECURSIVE 互斥锁 ===\n";

    iox_pthread_mutexattr_t attr;
    CHECK(iox_pthread_mutexattr_init(&attr));
    CHECK(iox_pthread_mutexattr_settype(&attr, IOX_PTHREAD_MUTEX_RECURSIVE));

    iox_pthread_mutex_t mtx;
    CHECK(iox_pthread_mutex_init(&mtx, &attr));
    CHECK(iox_pthread_mutexattr_destroy(&attr));

    recursiveLock(&mtx, 3);  // 同一线程重入 3 次，不会死锁

    CHECK(iox_pthread_mutex_destroy(&mtx));
    std::cout << "  ✓ 递归加锁/解锁全部成功\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// DEMO 4  trylock：非阻塞尝试加锁
// ─────────────────────────────────────────────────────────────────────────────

// 必须用 std::atomic<bool>：普通 bool 跨线程自旋没有内存屏障，
// 编译器可将 while(!flag){} 优化为只读一次寄存器，导致主线程
// 在子线程真正持锁前就执行 trylock，得到意外的 rc=0。
struct TrylockArg {
    iox_pthread_mutex_t* mtx;
    std::atomic<bool>*   holderReady;
    std::atomic<bool>*   done;
};

static void* holderFunc(void* arg) {
    auto* a = static_cast<TrylockArg*>(arg);
    CHECK(iox_pthread_mutex_lock(a->mtx));
    a->holderReady->store(true, std::memory_order_release);  // 通知主线程：锁已被持有
    while (!a->done->load(std::memory_order_acquire)) {
    }  // 等主线程 trylock 失败后再释放
    CHECK(iox_pthread_mutex_unlock(a->mtx));
    return nullptr;
}

static void demo_trylock() {
    std::cout << "\n=== DEMO 4: trylock ===\n";

    iox_pthread_mutex_t mtx;
    CHECK(iox_pthread_mutex_init(&mtx, nullptr));  // nullptr → 默认属性

    std::atomic<bool> holderReady{false}, done{false};
    TrylockArg        arg{&mtx, &holderReady, &done};

    iox_pthread_t t;
    CHECK(iox_pthread_create(&t, nullptr, holderFunc, &arg));

    // 等子线程持锁（acquire 配合子线程的 release store）
    while (!holderReady.load(std::memory_order_acquire)) {
    }

    // trylock 应失败（锁被占用）
    int rc = iox_pthread_mutex_trylock(&mtx);
    std::cout << "  trylock while locked: rc=" << rc << " (" << std::strerror(rc)
              << ")  期望 EBUSY=" << EBUSY << "\n";
    assert(rc == EBUSY);

    done.store(true, std::memory_order_release);  // 通知子线程可以释放锁了
    CHECK(iox_pthread_join(t, nullptr));

    // 子线程已释放，trylock 应成功
    rc = iox_pthread_mutex_trylock(&mtx);
    std::cout << "  trylock after release: rc=" << rc << (rc == 0 ? "  ✓ 加锁成功" : "  ✗ 意外失败")
              << "\n";
    assert(rc == 0);
    CHECK(iox_pthread_mutex_unlock(&mtx));

    CHECK(iox_pthread_mutex_destroy(&mtx));
}

// ─────────────────────────────────────────────────────────────────────────────
// DEMO 5  优先级继承协议（PRIO_INHERIT）
//         展示属性设置路径；实际优先级反转效果需实时调度器，这里只做配置演示
// ─────────────────────────────────────────────────────────────────────────────

static void demo_prio_inherit() {
    std::cout << "\n=== DEMO 5: PRIO_INHERIT 优先级继承属性配置 ===\n";

    iox_pthread_mutexattr_t attr;
    CHECK(iox_pthread_mutexattr_init(&attr));
    CHECK(iox_pthread_mutexattr_setprotocol(&attr, IOX_PTHREAD_PRIO_INHERIT));

    iox_pthread_mutex_t mtx;
    int                 rc = iox_pthread_mutex_init(&mtx, &attr);
    CHECK(iox_pthread_mutexattr_destroy(&attr));

    if (rc == 0) {
        std::cout << "  ✓ PRIO_INHERIT 互斥锁创建成功\n";
        CHECK(iox_pthread_mutex_lock(&mtx));
        CHECK(iox_pthread_mutex_unlock(&mtx));
        CHECK(iox_pthread_mutex_destroy(&mtx));
    } else {
        // 非实时内核可能返回 ENOTSUP
        std::cout << "  ！ iox_pthread_mutex_init rc=" << rc << " (" << std::strerror(rc)
                  << ")  可能需要实时调度支持\n";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// DEMO 6  健壮互斥锁（ROBUST + ERRORCHECK）：持锁线程异常退出后的恢复
// ─────────────────────────────────────────────────────────────────────────────

static iox_pthread_mutex_t g_robustMtx;

static void* crashHolder(void* /*arg*/) {
    CHECK(iox_pthread_mutex_lock(&g_robustMtx));
    std::cout << "  [crashHolder] 持锁后直接退出（模拟崩溃）\n";
    // 故意不调用 unlock，线程终止后内核将互斥锁标记为 EOWNERDEAD
    return nullptr;
}

static void demo_robust_mutex() {
    std::cout << "\n=== DEMO 6: ROBUST 健壮互斥锁 ===\n";

    iox_pthread_mutexattr_t attr;
    CHECK(iox_pthread_mutexattr_init(&attr));
    CHECK(iox_pthread_mutexattr_setrobust(&attr, IOX_PTHREAD_MUTEX_ROBUST));
    // ERRORCHECK 让同一线程重复 lock 返回 EDEADLK 而非死锁，便于调试
    CHECK(iox_pthread_mutexattr_settype(&attr, IOX_PTHREAD_MUTEX_ERRORCHECK));

    CHECK(iox_pthread_mutex_init(&g_robustMtx, &attr));
    CHECK(iox_pthread_mutexattr_destroy(&attr));

    // 子线程持锁后退出
    iox_pthread_t t;
    CHECK(iox_pthread_create(&t, nullptr, crashHolder, nullptr));
    CHECK(iox_pthread_join(t, nullptr));

    // 主线程尝试加锁，应得到 EOWNERDEAD
    int rc = iox_pthread_mutex_lock(&g_robustMtx);
    std::cout << "  lock after owner exit: rc=" << rc;
    if (rc == EOWNERDEAD) {
        std::cout << " (EOWNERDEAD)  ✓\n";
        // 将互斥锁状态恢复为一致
        CHECK(iox_pthread_mutex_consistent(&g_robustMtx));
        std::cout << "  iox_pthread_mutex_consistent() 调用成功，互斥锁已恢复\n";
        CHECK(iox_pthread_mutex_unlock(&g_robustMtx));
    } else {
        std::cout << " (" << std::strerror(rc) << ")  ？非预期返回值\n";
    }

    CHECK(iox_pthread_mutex_destroy(&g_robustMtx));
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main() {
    demo_thread();
    demo_normal_mutex();
    demo_recursive_mutex();
    demo_trylock();
    demo_prio_inherit();
    demo_robust_mutex();

    std::cout << "\n=== 全部 Demo 完成 ===\n";
    return 0;
}
