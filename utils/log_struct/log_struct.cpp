#include "log_struct.hpp"

#include <atomic>

namespace utils {
LogStruct::LogStruct(const std::string& log_path, std::size_t struct_size, std::size_t reverse_size,
                     std::size_t polling_interval)
    : log_path_(log_path),
      struct_size_(struct_size),
      byte_size_(struct_size * reverse_size),
      polling_interval_(polling_interval),
      raw_buffer1_(new std::byte[byte_size_ + 1024]),  // 多分一点 Buffer 给 Vector 内部开销
      raw_buffer2_(new std::byte[byte_size_ + 1024]),
      pool1_(raw_buffer1_, byte_size_ + 1024),
      pool2_(raw_buffer2_, byte_size_ + 1024),
      vector1_(&pool1_),
      vector2_(&pool2_) {
    fd_ = ::open(log_path_.c_str(), O_RDWR | O_CREAT | O_APPEND, 0666);
    if (fd_ == -1)
        throw std::runtime_error("Open failed");

    vector1_.reserve(byte_size_);
    vector2_.reserve(byte_size_);

    current_vector_.store(&vector1_);
    next_vector_.store(&vector2_);
    last_write_time_ = std::chrono::system_clock::now();

    write_thread_ = std::jthread([this](std::stop_token st) { writeToFile(st); });
}

LogStruct::~LogStruct() {
    // 1. 停止接收新数据
    is_running_.store(false);

    // 2. 强制触发最后一轮写入
    flush();

    // 3. jthread 析构会自动发送 stop 信号并 join
    if (fd_ != -1) {
        ::fsync(fd_);
        ::close(fd_);
    }
    delete[] raw_buffer1_;
    delete[] raw_buffer2_;
}

bool LogStruct::flush() {
    if (is_writing_.load(std::memory_order_relaxed)) {
        std::cerr << "flush while writing" << std::endl;
        return false;
    }

    is_writing_.store(true, std::memory_order_relaxed);

    // 手动触发 Ping-Pong 切换
    // is_switching_.store(true, std::memory_order_seq_cst);
    auto old_current = current_vector_.load(std::memory_order_relaxed);
    auto old_next    = next_vector_.load(std::memory_order_relaxed);

    current_vector_.store(old_next, std::memory_order_relaxed);
    next_vector_.store(old_current, std::memory_order_relaxed);
    // is_switching_.store(false, std::memory_order_release);

    has_data_to_write.test_and_set(std::memory_order_release);
    has_data_to_write.notify_one();

    return true;
}

bool LogStruct::write(const std::byte* data) {
    // if (is_switching_.load(std::memory_order_acquire)) {
    //     std::cerr << "write while switching" << std::endl;
    //     return false;
    // }

    auto curr = current_vector_.load(std::memory_order_relaxed);

    if (data) {
        if (curr->size() + struct_size_ > byte_size_)
            return false;
        curr->insert(curr->end(), data, data + struct_size_);
    }

    // 检查时间或容量是否达到阈值
    auto now = std::chrono::system_clock::now();
    if (now - last_write_time_ >= std::chrono::milliseconds(polling_interval_) ||
        curr->size() >= byte_size_) {
        if (flush()) {
            last_write_time_ = now;
        }
    }
    return true;
}

void LogStruct::writeToFile(std::stop_token st) {
    while (!st.stop_requested() || has_data_to_write.test()) {
        // 等待信号，如果 flag 为 false 则阻塞
        has_data_to_write.wait(false, std::memory_order_acquire);

        auto target = next_vector_.load();

        if (!target->empty()) {
            std::size_t need_write_size = target->size();
            std::size_t written_size    = 0;
            while (written_size < need_write_size) {
                ssize_t result =
                    ::write(fd_, target->data() + written_size, need_write_size - written_size);
                if (result < 0) {
                    break;
                }
                written_size += static_cast<std::size_t>(result);
            }
            target->resize(0);
        }

        is_writing_.store(false, std::memory_order_relaxed);
        has_data_to_write.clear(std::memory_order_release);

        // 如果是因为退出而进行的最后一轮，在这里跳出
        if (!is_running_.load(std::memory_order_relaxed) && current_vector_.load()->empty())
            break;
    }
}
}  // namespace utils
