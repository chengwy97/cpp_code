#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <functional>
#include <optional>

/*
# `018.check_state` 设计说明

## 目标

对外部周期性输入的状态做归约，输出一个更稳定的状态值。

当前约定的输入状态分为三类：

- `0`：空闲状态
- `1..4`：业务状态
- `6..N`：直通状态，不参与确认窗口

## 接口语义

当前实现对外提供：

```cpp
int push(int state);
```

返回值含义：

- 返回 `5`：确认窗口内业务状态次数达到阈值
- 返回 `1..4`：直接输出某个业务状态
- 返回 `6..N`：直通状态立即输出
- 返回 `0`：
  - 当前没有形成特殊输出
  - 或折叠结果本身就是 `0`

说明：

- 当前实现将“无输出”和“输出 0”统一表示为 `0`
- `push()` 内部使用调用瞬间作为当前时间

## 确认窗口

只有 `0..4` 会进入确认窗口。

窗口规则如下：

1. 第一次收到 `0..4` 时开启窗口
2. 在窗口有效期内继续接收后续 `0..4`
3. 只有 `1..4` 会计入业务次数
4. 当窗口内业务次数达到阈值 `m` 时，输出 `5`，并清空窗口

其中：

- 窗口时长为 `n`
- 业务阈值为 `m`

## 超时规则

当下一次调用 `push()` 时，如果发现当前窗口已经超过确认期：

### 情况 1：最新一帧是业务状态 `1..4`

直接输出当前最新业务状态。

此时旧窗口被丢弃，不再输出旧窗口折叠结果。

对应实现语义：

```cpp
if (isExpired(now)) {
    clearPending();
    return state; // state in [1..4]
}
```

### 情况 2：最新一帧是 `0`

先对旧窗口做折叠，再输出折叠结果：

- 如果旧窗口里出现过业务状态，输出最后一个业务状态
- 如果旧窗口里没有业务状态，输出 `0`

对应实现语义：

```cpp
if (isExpired(now)) {
    int output = foldPending();
    clearPending();
    return output; // state == 0
}
```

## 直通状态规则

当收到 `6..N`：

1. 立即输出该状态
2. 清空当前待确认窗口
3. 不参与任何折叠和计数

## 折叠规则

折叠函数定义为：

- 如果窗口中出现过业务状态，则输出最后一个业务状态
- 否则输出 `0`

当前实现对应：

```cpp
int foldPending() const { return last_business_state_.value_or(0); }
```

## 典型时序

### 1. 窗口内达到阈值，输出 `5`

参数：

- `n = 1000ms`
- `m = 3`

输入：

- `t=0 -> 1`
- `t=100 -> 0`
- `t=300 -> 2`
- `t=700 -> 4`

输出：

- `t=700 -> 5`

原因：

- 窗口内业务状态 `1, 2, 4` 共 3 次，达到阈值

### 2. 超时后最新帧为 `0`，输出旧窗口折叠结果

输入：

- `t=0 -> 1`
- `t=100 -> 0`
- `t=400 -> 2`
- `t=1000 -> 0`

输出：

- `t=1000 -> 2`

原因：

- 旧窗口超时
- 最新帧是 `0`
- 旧窗口中最后一个业务状态为 `2`

### 3. 超时后最新帧为业务状态，直接输出最新业务状态

输入：

- `t=0 -> 1`
- `t=100 -> 0`
- `t=400 -> 2`
- `t=1000 -> 3`

输出：

- `t=1000 -> 3`

原因：

- 旧窗口超时
- 最新帧 `3` 是业务状态
- 直接以最新状态为准，旧窗口丢弃

### 4. 只有空闲状态，超时后输出 `0`

输入：

- `t=0 -> 0`
- `t=200 -> 0`
- `t=1000 -> 0`

输出：

- `t=1000 -> 0`

### 5. 直通状态打断窗口

输入：

- `t=0 -> 1`
- `t=100 -> 0`
- `t=200 -> 6`

输出：

- `t=200 -> 6`

原因：

- `6` 为直通状态
- 当前窗口立即清空

## 当前实现的重要取舍

当前实现采用了一个简化约定：

- 如果窗口超时且当前最新帧是业务状态 `1..4`
- 该帧会被“直接输出并消费”
- 不会在输出后继续作为新窗口第一帧保留下来

也就是说，下面这种场景中：

- `t=1200 -> 3` 因超时被直接输出

那么后续不会再把这次 `3` 当成下一轮窗口的起点。

如果后续业务需要“既立即输出当前业务帧，又把它作为新窗口第一帧继续累计”，则需要调整现有实现。

*/

class StateReducer {
   public:
    using Clock       = std::chrono::steady_clock;
    using TimePoint   = Clock::time_point;
    using NowProvider = std::function<TimePoint()>;

    StateReducer(
        std::chrono::milliseconds confirm_window, std::size_t business_threshold,
        NowProvider now_provider = [] { return Clock::now(); })
        : confirm_window_(confirm_window),
          business_threshold_(business_threshold),
          now_provider_(std::move(now_provider)) {}

    int push(int state) {
        const auto now = now_provider_();

        if (state >= 6) {
            clearPending();
            return state;
        }

        bool expired = false;
        int  output  = 0;
        if (isExpired(now)) {
            expired = true;
            output  = foldPending();
            clearPending();
            return state != 0 ? state : output;
        }

        appendPending(state, now);

        if (business_count_ >= business_threshold_) {
            clearPending();
            return 5;
        }

        return expired ? output : 0;
    }

   private:
    void appendPending(int state, TimePoint now) {
        if (!window_start_.has_value()) {
            window_start_ = now;
        }

        if (state >= 1 && state <= 4) {
            ++business_count_;
            last_business_state_ = state;
        }
    }

    bool isExpired(TimePoint now) const {
        if (!window_start_.has_value()) {
            return false;
        }

        return now - *window_start_ >= confirm_window_;
    }

    int foldPending() const { return last_business_state_.value_or(0); }

    void clearPending() {
        window_start_.reset();
        business_count_ = 0;
        last_business_state_.reset();
    }

   private:
    std::chrono::milliseconds confirm_window_;
    std::size_t               business_threshold_;
    NowProvider               now_provider_;
    std::optional<TimePoint>  window_start_;
    std::size_t               business_count_{0};
    std::optional<int>        last_business_state_;
};

namespace {
StateReducer::TimePoint ms(const int value) {
    return StateReducer::TimePoint{std::chrono::milliseconds(value)};
}
}  // namespace

TEST(StateReducerTest, OutputFiveWhenThresholdReachedWithinWindow) {
    StateReducer::TimePoint now;
    StateReducer            reducer(std::chrono::milliseconds(1000), 3, [&] { return now; });

    now = ms(0);
    EXPECT_EQ(reducer.push(1), 0);
    now = ms(100);
    EXPECT_EQ(reducer.push(0), 0);
    now = ms(300);
    EXPECT_EQ(reducer.push(2), 0);

    now = ms(700);
    EXPECT_EQ(reducer.push(4), 5);
}

TEST(StateReducerTest, TimeoutWithIdleFrameOutputsFoldedBusinessState) {
    StateReducer::TimePoint now;
    StateReducer            reducer(std::chrono::milliseconds(1000), 3, [&] { return now; });

    now = ms(0);
    EXPECT_EQ(reducer.push(1), 0);
    now = ms(100);
    EXPECT_EQ(reducer.push(0), 0);
    now = ms(400);
    EXPECT_EQ(reducer.push(2), 0);

    now = ms(1000);
    EXPECT_EQ(reducer.push(0), 2);
}

TEST(StateReducerTest, TimeoutWithBusinessFrameOutputsLatestBusinessState) {
    StateReducer::TimePoint now;
    StateReducer            reducer(std::chrono::milliseconds(1000), 3, [&] { return now; });

    now = ms(0);
    EXPECT_EQ(reducer.push(1), 0);
    now = ms(100);
    EXPECT_EQ(reducer.push(0), 0);
    now = ms(400);
    EXPECT_EQ(reducer.push(2), 0);

    now = ms(1000);
    EXPECT_EQ(reducer.push(3), 3);
}

TEST(StateReducerTest, TimeoutOutputsZeroWhenOnlyIdleStatesExist) {
    StateReducer::TimePoint now;
    StateReducer            reducer(std::chrono::milliseconds(1000), 3, [&] { return now; });

    now = ms(0);
    EXPECT_EQ(reducer.push(0), 0);
    now = ms(200);
    EXPECT_EQ(reducer.push(0), 0);

    now = ms(1000);
    EXPECT_EQ(reducer.push(0), 0);
}

TEST(StateReducerTest, DirectStatePassesThroughAndClearsPendingWindow) {
    StateReducer::TimePoint now;
    StateReducer            reducer(std::chrono::milliseconds(1000), 3, [&] { return now; });

    now = ms(0);
    EXPECT_EQ(reducer.push(1), 0);
    now = ms(100);
    EXPECT_EQ(reducer.push(0), 0);

    now = ms(200);
    EXPECT_EQ(reducer.push(6), 6);
}

TEST(StateReducerTest, LatestBusinessStateIsConsumedImmediatelyAfterTimeout) {
    StateReducer::TimePoint now;
    StateReducer            reducer(std::chrono::milliseconds(1000), 3, [&] { return now; });

    now = ms(0);
    EXPECT_EQ(reducer.push(1), 0);
    now = ms(200);
    EXPECT_EQ(reducer.push(2), 0);

    now = ms(1200);
    EXPECT_EQ(reducer.push(3), 3);

    now = ms(2199);
    EXPECT_EQ(reducer.push(0), 0);
    now = ms(2200);
    EXPECT_EQ(reducer.push(0), 0);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
