#include <iostream>
#include <variant>
#include <string>
#include <stdexcept>
#include <string_view>
#include <thread>

#include <future>

#include <barrier>
#include <latch>
#include <thread>
#include <atomic>
#include <vector>
#include <cassert>

std::latch latch(100);

std::atomic<unsigned long> counter = 0;

void worker(int id) {
    latch.arrive_and_wait();

    for (int i = 0; i < 1000000; i++) {
        counter.fetch_add(1, std::memory_order_relaxed);
    }
}
int main(int argc, char* argv[]) {
    std::vector<std::thread> threads;
    for (int i = 0; i < 100; i++) {
        threads.emplace_back(worker, i);
    }
    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "counter: " << counter.load(std::memory_order_relaxed) << std::endl;

    if (counter.load(std::memory_order_relaxed) != 100000000) {
        std::cout << "counter is not 100000000" << std::endl;
        return -1;
    }

    std::cout << "counter is 100000000" << std::endl;
    return 0;
}
