// Roudi：创建 N 个数据段 + 1 个控制段（全部清零），
//        监听 UDS，向客户端发送全部段信息（含控制段）

#include "003.common.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

static constexpr uint32_t NUM_DATA_SEGMENTS = 4;
static constexpr uint64_t DATA_SEGMENT_SIZE = 4096;  // 每个数据段 4 KB
static constexpr uint64_t CTRL_SEGMENT_SIZE = 4096;  // 控制段 4 KB（ControlMessage 极小）

// ─────────────────────────────────────────────────────────────────────────────

struct ShmSegment {
    SegmentInfo info;
    void*       addr{nullptr};
    int         fd{-1};
};

static std::vector<ShmSegment> g_segments;  // [0] = 控制段，[1..N] = 数据段
static int                     g_serverFd = -1;

// ─────────────────────────────────────────────────────────────────────────────

static void cleanup() {
    if (g_serverFd >= 0) { ::close(g_serverFd); g_serverFd = -1; }
    ::unlink(ROUDI_SOCK_PATH);

    for (auto& seg : g_segments) {
        if (seg.addr && seg.addr != MAP_FAILED) ::munmap(seg.addr, seg.info.size);
        if (seg.fd >= 0) ::close(seg.fd);
        ::shm_unlink(seg.info.shm_name);
    }
}

static void sighandler(int) { cleanup(); _exit(0); }

// ─────────────────────────────────────────────────────────────────────────────

static bool createSegment(ShmSegment& seg) {
    ::shm_unlink(seg.info.shm_name);  // 清理可能遗留的同名段

    seg.fd = ::shm_open(seg.info.shm_name, O_CREAT | O_RDWR, 0666);
    if (seg.fd < 0) { perror("shm_open"); return false; }

    if (::ftruncate(seg.fd, static_cast<off_t>(seg.info.size)) < 0) {
        perror("ftruncate"); return false;
    }

    seg.addr = ::mmap(nullptr, seg.info.size, PROT_READ | PROT_WRITE, MAP_SHARED, seg.fd, 0);
    if (seg.addr == MAP_FAILED) { perror("mmap"); return false; }

    std::memset(seg.addr, 0, seg.info.size);  // 全部初始化为 0
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────

static void handleClient(int clientFd) {
    uint8_t req = 0;
    if (::read(clientFd, &req, 1) != 1 || req != REQ_GET_SEGMENTS) {
        std::cerr << "[Roudi] Unknown request, closing\n";
        ::close(clientFd);
        return;
    }

    std::cout << "[Roudi] Client connected, sending " << g_segments.size() << " segments\n";

    uint32_t count = static_cast<uint32_t>(g_segments.size());
    ::write(clientFd, &count, sizeof(count));

    for (const auto& seg : g_segments) {
        ::write(clientFd, &seg.info, sizeof(SegmentInfo));
        std::cout << "[Roudi]   -> id=" << seg.info.id
                  << "  name=" << seg.info.shm_name
                  << "  size=" << seg.info.size
                  << (seg.info.is_control ? "  [CONTROL]" : "") << "\n";
    }

    ::close(clientFd);
}

// ─────────────────────────────────────────────────────────────────────────────

int main() {
    ::signal(SIGINT, sighandler);
    ::signal(SIGTERM, sighandler);

    // ── 1. 创建控制段（id = 0）─────────────────────────────────────────────
    {
        ShmSegment ctrl{};
        ctrl.info.id         = 0;
        ctrl.info.size       = CTRL_SEGMENT_SIZE;
        ctrl.info.is_control = 1;
        std::snprintf(ctrl.info.shm_name, sizeof(ctrl.info.shm_name), "/cpp_003_ctrl");

        if (!createSegment(ctrl)) { cleanup(); return 1; }

        std::cout << "[Roudi] Created control segment: id=0"
                  << "  name=" << ctrl.info.shm_name
                  << "  size=" << ctrl.info.size << "\n";
        g_segments.push_back(ctrl);
    }

    // ── 2. 创建数据段（id = 1..N）─────────────────────────────────────────
    for (uint32_t i = 0; i < NUM_DATA_SEGMENTS; ++i) {
        ShmSegment seg{};
        seg.info.id         = i + 1;
        seg.info.size       = DATA_SEGMENT_SIZE;
        seg.info.is_control = 0;
        std::snprintf(seg.info.shm_name, sizeof(seg.info.shm_name), "/cpp_003_seg_%u", i);

        if (!createSegment(seg)) { cleanup(); return 1; }

        std::cout << "[Roudi] Created data segment:    id=" << seg.info.id
                  << "  name=" << seg.info.shm_name
                  << "  size=" << seg.info.size << "\n";
        g_segments.push_back(seg);
    }

    // ── 3. 创建 UDS 服务端 ─────────────────────────────────────────────────
    ::unlink(ROUDI_SOCK_PATH);

    g_serverFd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (g_serverFd < 0) { perror("socket"); cleanup(); return 1; }

    sockaddr_un uAddr{};
    uAddr.sun_family = AF_UNIX;
    std::strncpy(uAddr.sun_path, ROUDI_SOCK_PATH, sizeof(uAddr.sun_path) - 1);

    if (::bind(g_serverFd, reinterpret_cast<sockaddr*>(&uAddr), sizeof(uAddr)) < 0) {
        perror("bind"); cleanup(); return 1;
    }
    if (::listen(g_serverFd, 16) < 0) { perror("listen"); cleanup(); return 1; }

    std::cout << "\n[Roudi] Listening on " << ROUDI_SOCK_PATH << "\n";
    std::cout << "[Roudi] Press Ctrl+C to exit\n\n";

    // ── 4. 循环接受客户端 ──────────────────────────────────────────────────
    while (true) {
        int clientFd = ::accept(g_serverFd, nullptr, nullptr);
        if (clientFd < 0) {
            if (errno == EINTR) break;
            perror("accept");
            continue;
        }
        handleClient(clientFd);
    }

    cleanup();
    return 0;
}
