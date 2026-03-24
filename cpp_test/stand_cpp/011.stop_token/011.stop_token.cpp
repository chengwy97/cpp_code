#include <chrono>
#include <functional>
#include <iostream>
#include <list>
#include <stop_token>
#include <thread>
#include <utility>

class ThreadPool {
    struct PrivateTag {};

   public:
    static ThreadPool& instance() {
        static ThreadPool pool(PrivateTag{});
        return pool;
    }

    ThreadPool(PrivateTag) {}
    bool add_task(std::string_view thread_name, std::function<void(std::stop_token)> task) {
        if (!is_running_.load(std::memory_order_relaxed)) {
            return false;
        }

        std::lock_guard<std::mutex> lock(workers_mutex_);
        auto                        stop_source = std::make_unique<std::stop_source>();
        auto                        stop_token  = stop_source->get_token();
        workers_.emplace_back(
            std::make_pair(std::move(stop_source),
                           std::make_unique<std::jthread>([this, thread_name, stop_token, task]() {
                               execute_task(thread_name, stop_token, task);
                           })));
        return true;
    }

    void stop() {
        is_running_.store(false, std::memory_order_relaxed);
        std::lock_guard<std::mutex> lock(workers_mutex_);
        for (auto& [stop_source, thread] : workers_) {
            stop_source->request_stop();
            thread->join();
        }

        workers_.clear();
    }

   private:
    std::atomic<bool> is_running_ = true;
    std::mutex        workers_mutex_;
    std::list<std::pair<std::unique_ptr<std::stop_source>, std::unique_ptr<std::jthread>>> workers_;

    void execute_task(std::string_view thread_name, std::stop_token stop_token,
                      std::function<void(std::stop_token)> task) {
        // 设置线程名称
        pthread_setname_np(pthread_self(), thread_name.data());
        // 执行任务
        task(stop_token);
    }
};

void test_stop_token(std::stop_token stop_token) {
    while (!stop_token.stop_requested()) {
        std::cout << "Running..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "Stop requested" << std::endl;
}

class A {
   public:
    A() { std::cout << "A constructor" << std::endl; }
    ~A() { std::cout << "A destructor" << std::endl; }
    void test() { std::cout << "A test" << std::endl; }
};

class B {
   public:
    B() { std::cout << "B constructor" << std::endl; }
    ~B() { std::cout << "B destructor" << std::endl; }
    A test() { return A(); }
};

int main() {
    auto& thread_pool = ThreadPool::instance();
    thread_pool.add_task("test", test_stop_token);
    thread_pool.add_task("test", test_stop_token);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    thread_pool.stop();

    std::cout << "completed" << std::endl;

    B().test().test();

    return 0;
}
