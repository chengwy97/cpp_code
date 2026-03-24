#include <atomic>
#include <boost/stacktrace.hpp>
#include <csignal>
#include <cstdlib>
#include <iostream>

#include "logging/log_def.hpp"

namespace {

std::atomic_bool g_in_handler{false};

void core_signal_handler(int signal_number, siginfo_t* signal_info, [[maybe_unused]] void* uctx) {
    if (g_in_handler.exchange(true)) {
        std::_Exit(128 + signal_number);
    }

    UTILS_LOG_FATAL("Captured coredump signal: {}", signal_number);
    if (signal_info != nullptr) {
        UTILS_LOG_ERROR("signal_code={}, signal_addr={}", signal_info->si_code,
                        static_cast<const void*>(signal_info->si_addr));
    }

    UTILS_LOG(ERROR) << "Stacktrace:\n" << boost::stacktrace::stacktrace();
    UTILS_LOG_FLUSH();

    // Restore default behavior and raise again to generate the core dump.
    std::signal(signal_number, SIG_DFL);
    std::raise(signal_number);
}

void install_core_signal_handler(int signal_number) {
    struct sigaction action{};
    action.sa_sigaction = &core_signal_handler;
    action.sa_flags     = SA_SIGINFO | SA_RESETHAND;
    sigemptyset(&action.sa_mask);

    if (sigaction(signal_number, &action, nullptr) != 0) {
        std::cerr << "Failed to install signal handler for signal " << signal_number << std::endl;
    }
}

void install_core_signal_handlers() {
    install_core_signal_handler(SIGABRT);
    install_core_signal_handler(SIGSEGV);
    install_core_signal_handler(SIGILL);
    install_core_signal_handler(SIGFPE);
#ifdef SIGBUS
    install_core_signal_handler(SIGBUS);
#endif
}

}  // namespace

void b1();
void b2();
void b3();
void b4();
void b5();
void b6();
void b7();
void b8();
void b9();
void b10();

void b1() {
    std::raise(SIGABRT);
}

void b2() {
    b1();
}

void b3() {
    b2();
}

void b4() {
    b3();
}

void b5() {
    b4();
}

void b6() {
    b5();
}

void b7() {
    b6();
}

void b8() {
    b7();
}

void b9() {
    b8();
}

void b10() {
    b9();
}

int main() {
    int ret = UTILS_LOG_INIT("/tmp/test.log", "debug");
    if (ret != 0) {
        std::cerr << "Failed to initialize logging" << std::endl;
        return 1;
    }

    install_core_signal_handlers();

    UTILS_LOG_INFO("Installed core signal handlers. Triggering SIGABRT for demo.");
    UTILS_LOG_FLUSH();

    // Demo: raise a coredump signal.
    b10();

    return 0;
}
