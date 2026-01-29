#include <fcntl.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <format>
#include <iostream>
#include <memory_resource>
#include <string>
#include <thread>
#include <vector>

namespace utils {

class LogStruct {
   private:
    // 配置参数
    std::string log_path_;
    std::size_t struct_size_;
    std::size_t byte_size_;
    std::size_t polling_interval_;

    // 状态控制
    int                                   fd_ = -1;
    std::atomic<bool>                     is_running_{true};
    std::atomic<bool>                     is_writing_{false};
    std::atomic_flag                      has_data_to_write = ATOMIC_FLAG_INIT;
    std::chrono::system_clock::time_point last_write_time_;

    // 内存管理：Ping-Pong Buffer
    std::byte*                          raw_buffer1_;
    std::byte*                          raw_buffer2_;
    std::pmr::monotonic_buffer_resource pool1_;
    std::pmr::monotonic_buffer_resource pool2_;

    // 业务容器
    std::pmr::vector<std::byte> vector1_;
    std::pmr::vector<std::byte> vector2_;

    // 指针切换（原子操作）
    std::atomic<std::pmr::vector<std::byte>*> current_vector_;
    std::atomic<std::pmr::vector<std::byte>*> next_vector_;

    // std::atomic<bool> is_switching_ = false;

    std::jthread write_thread_;

   public:
    LogStruct(const std::string& log_path, std::size_t struct_size, std::size_t reverse_size,
              std::size_t polling_interval);

    ~LogStruct();

    bool flush();

    bool write(const std::byte* data);

   private:
    void writeToFile(std::stop_token st);
};

}  // namespace utils
