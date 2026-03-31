#include <gsl/assert>
#include <gsl/gsl>
#include <iostream>
#include <iox/atomic.hpp>
#include <iox/detail/semaphore_helper.hpp>
#include <iox/error_reporting/macros.hpp>
#include <iox/optional.hpp>
#include <iox/spin_semaphore.hpp>
#include <thread>

using namespace iox;

optional<concurrent::SpinSemaphore> g_spin_semaphore;

concurrent::Atomic<int> g_count(0);

void post_spin_semaphore() {
    Expects(g_count.load(std::memory_order_relaxed) == 0);
    g_count.store(10, std::memory_order_relaxed);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    g_spin_semaphore->post()
        .and_then([]() { std::cout << "post spin semaphore success" << std::endl; })
        .or_else([](auto& error) {
            std::cout << "post spin semaphore failed: " << static_cast<int>(error) << std::endl;
        });
}

void wait_spin_semaphore() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    g_spin_semaphore->wait()
        .and_then([]() { std::cout << "wait spin semaphore success" << std::endl; })
        .or_else([](auto& error) {
            std::cout << "wait spin semaphore failed: " << static_cast<int>(error) << std::endl;
        });
    Ensures(g_count.load(std::memory_order_relaxed) == 10);
    g_count.fetch_sub(1, std::memory_order_relaxed);
}

int main() {
    concurrent::SpinSemaphoreBuilder()
        .create(g_spin_semaphore)
        .and_then([]() { std::cout << "create spin semaphore success" << std::endl; })
        .or_else([](auto& error) {
            std::cout << "create spin semaphore failed: " << static_cast<int>(error) << std::endl;
        });

    std::thread t1(post_spin_semaphore);
    std::thread t2(wait_spin_semaphore);
    t1.join();
    t2.join();

    std::cout << "g_count: " << g_count.load(std::memory_order_relaxed) << std::endl;
    Ensures(g_count.load(std::memory_order_relaxed) == 9);
    return 0;
}
