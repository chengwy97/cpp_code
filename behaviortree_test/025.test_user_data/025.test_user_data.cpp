#include <iostream>
#include <logging/log_def.hpp>
#include <task.hpp>
#include <thread_pool.hpp>

class TestTask : public task::Task<int> {
   public:
    TestTask(const std::string& name) : task::Task<int>(name) {}

    void end_task(int value) {
        if (value > 10) {
            final_result_ = task::TaskResult::SUCCESS;
        } else {
            final_result_ = task::TaskResult::FAILURE;
        }

        is_end = true;

        resume();
    }

    bool when_pause() override {
        UTILS_LOG_INFO("TestTask when_pause");
        return true;
    }

    bool when_resume() override {
        UTILS_LOG_INFO("TestTask when_resume");
        return true;
    }

    bool when_cancel() override {
        UTILS_LOG_INFO("TestTask when_cancel");
        return true;
    }

    boost::asio::awaitable<task::TaskResult> run_coroutine() override {
        co_await boost::asio::this_coro::executor;
        UTILS_LOG_INFO("TestTask running");

        while (!is_end) {
            if (is_cancelled()) {
                while (!when_cancel()) {
                }
                UTILS_LOG_INFO("TestTask is cancelled");
                co_return task::TaskResult::CANCELLED;
            }

            UTILS_LOG_INFO("TestTask is waiting for resume");
            if (!co_await async_wait_for_resume()) {
                UTILS_LOG_INFO("TestTask is not paused, no need to wait for resume");
                co_return task::TaskResult::CANCELLED;
            }

            UTILS_LOG_INFO("TestTask is waiting for input parameters");
            auto value = co_await wait_for_input_parameters();
            if (!value) {
                UTILS_LOG_ERROR("TestTask received input parameters error: {}", value.error());
                continue;
            }
            UTILS_LOG_INFO("TestTask received value: {}", value.value());
        }

        UTILS_LOG_INFO("TestTask completed");
        co_return final_result_;
    }

   private:
    std::atomic<task::TaskResult> final_result_ = task::TaskResult::SUCCESS;
    std::atomic_bool              is_end        = false;
};

int main() {
    UTILS_LOG_INIT("test.log", "debug");

    std::shared_ptr<TestTask> test_task = std::make_shared<TestTask>("TestTask");
    test_task->run();

    int i = 0;

    auto status = test_task->get_current_status();
    UTILS_LOG_INFO("TestTask initial status: {}", task::to_string(status));
    while (status != task::TaskStatus::COMPLETED && status != task::TaskStatus::FAILED &&
           status != task::TaskStatus::CANCELLED && i++ < 10) {
        // UTILS_LOG_INFO("TestTask status: {}", task::to_string(status));

        // test_task->end_task(11);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        status = test_task->get_current_status();

        if (test_task->send_input_parameters(i)) {
            UTILS_LOG_INFO("TestTask sent input parameters: {}", i);
        } else {
            UTILS_LOG_ERROR("TestTask send input parameters failed");
        }
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    test_task->cancel();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    status = test_task->get_current_status();

    UTILS_LOG_INFO("TestTask completed, status: {}", task::to_string(status));

    UTILS_LOG_FLUSH();
    return 0;
}
