#include <iostream>
#include <logging/log_def.hpp>
#include <task.hpp>
#include <thread_pool.hpp>

// 先包含 BehaviorTree 的基础头文件以获取 StringView 类型
#include "behaviortree_cpp/blackboard.h"
#include "behaviortree_cpp/bt_factory.h"
#include "dummy_nodes.h"

class InitLog {
   public:
    InitLog() { UTILS_LOG_INIT("ota_state.log", "debug"); }
    ~InitLog() { UTILS_LOG_FLUSH(); }
};

static InitLog init_log;

enum OtaState {
    IDLE,
    DOWNLOADING,
    VERIFYING,
    UPDATING,
    COMPLETED,
    FAILED,
};

std::string to_string(OtaState state) {
    switch (state) {
        case OtaState::IDLE:
            return "IDLE";
        case OtaState::DOWNLOADING:
            return "DOWNLOADING";
        case OtaState::VERIFYING:
            return "VERIFYING";
        case OtaState::UPDATING:
            return "UPDATING";
        case OtaState::COMPLETED:
            return "COMPLETED";
        case OtaState::FAILED:
            return "FAILED";
        default:
            return "UNKNOWN";
    }
}

// 为 BehaviorTree 提供字符串到枚举的转换
// 必须在 BehaviorTree 头文件包含之后定义，以便使用 StringView 类型
namespace BT {
template <>
inline OtaState convertFromString<OtaState>(StringView str) {
    std::string state_str(str.data(), str.size());
    UTILS_LOG_INFO("Converting OtaState from string: [{}]", state_str);
    if (state_str == "IDLE") {
        return OtaState::IDLE;
    } else if (state_str == "DOWNLOADING") {
        return OtaState::DOWNLOADING;
    } else if (state_str == "VERIFYING") {
        return OtaState::VERIFYING;
    } else if (state_str == "UPDATING") {
        return OtaState::UPDATING;
    } else if (state_str == "COMPLETED") {
        return OtaState::COMPLETED;
    } else if (state_str == "FAILED") {
        return OtaState::FAILED;
    } else {
        throw RuntimeError("Invalid OtaState: " + state_str);
    }
}
}  // namespace BT

class OtaTask : public task::Task<OtaState> {
   public:
    OtaTask() : task::Task<OtaState>("OtaTask") {}

    ~OtaTask() {}

    boost::asio::awaitable<task::TaskResult> run_coroutine() override {
        for (;;) {
            if (is_cancelled()) {
                while (!when_cancel()) {
                    UTILS_LOG_INFO("OtaTask waiting for cancel");
                }
                co_return task::TaskResult::CANCELLED;
            }

            if (!co_await async_wait_for_resume()) {
                continue;
            }

            auto result = co_await wait_for_input_parameters();
            if (!result) {
                continue;
            }

            auto input_parameters = result.value();
            switch (input_parameters) {
                case OtaState::IDLE:
                    co_return task::TaskResult::SUCCESS;
                case OtaState::DOWNLOADING:
                case OtaState::VERIFYING:
                case OtaState::UPDATING:
                    continue;
                case OtaState::COMPLETED:
                    co_return task::TaskResult::SUCCESS;
                case OtaState::FAILED:
                    co_return task::TaskResult::FAILURE;
                default:
                    co_return task::TaskResult::FAILURE;
            }
        }
        co_return task::TaskResult::SUCCESS;
    }

    bool when_pause() override {
        UTILS_LOG_INFO("OtaTask when_pause");
        return true;
    }

    bool when_resume() override {
        UTILS_LOG_INFO("OtaTask when_resume");
        return true;
    }

    bool when_cancel() override {
        UTILS_LOG_INFO("OtaTask when_cancel");
        return true;
    }
};

class OtaNode : public BT::StatefulActionNode {
   public:
    static BT::PortsList providedPorts() { return {BT::InputPort<OtaState>("ota_state")}; }
    OtaNode(const std::string& name, const BT::NodeConfig& config)
        : BT::StatefulActionNode(name, config) {}
    ~OtaNode() { onHalted(); }
    BT::NodeStatus onStart() override {
        UTILS_LOG_INFO("OtaNode onStart");

        auto res = getInput<OtaState>("ota_state");
        if (!res) {
            UTILS_LOG_ERROR("OtaNode failed to get ota_state: {}", res.error());
            return BT::NodeStatus::FAILURE;
        }

        UTILS_LOG_INFO("OtaNode ota_state: {}", to_string(res.value()));
        auto ota_state = res.value();
        if (ota_state == OtaState::IDLE) {
            UTILS_LOG_INFO("OtaNode ota_state is IDLE");
            return BT::NodeStatus::FAILURE;
        } else {
            if (!ota_task_) {
                ota_task_ = std::make_shared<OtaTask>();
            }
            if (!ota_task_->run()) {
                return BT::NodeStatus::FAILURE;
            }
            return BT::NodeStatus::RUNNING;
        }
    }

    BT::NodeStatus onRunning() override {
        auto task_run_status = ota_task_->get_current_status();
        if (task_run_status == task::TaskStatus::COMPLETED) {
            return BT::NodeStatus::SUCCESS;
        } else if (task_run_status == task::TaskStatus::FAILED) {
            return BT::NodeStatus::FAILURE;
        } else {
            auto res = getInput<OtaState>("ota_state");
            if (!res) {
                UTILS_LOG_ERROR("OtaNode failed to get ota_state: {}", res.error());
                return BT::NodeStatus::FAILURE;
            }
            auto ota_state = res.value();
            if (ota_task_->send_input_parameters(ota_state)) {
                UTILS_LOG_INFO("OtaNode sent input parameters");
            } else {
                UTILS_LOG_ERROR("OtaNode send input parameters failed");
            }

            return BT::NodeStatus::RUNNING;
        }
        return BT::NodeStatus::SUCCESS;
    }
    void onHalted() override {
        if (ota_task_) {
            ota_task_->cancel();
            ota_task_.reset();
        }
    }

   private:
    std::shared_ptr<OtaTask> ota_task_;
};

static const char* xml_tree = R"(
<root BTCPP_format="4">
    <BehaviorTree ID="MainTree">
        <Sequence>
            <Script code=" ota_state:=DOWNLOADING " />
            <Fallback>
                <OtaNode name="check_ota_state" ota_state="{ota_state}" />
                <AlwaysSuccess name="success" />
            </Fallback>
        </Sequence>
    </BehaviorTree>
</root>
)";

int main() {
    BT::BehaviorTreeFactory factory;
    factory.registerNodeType<OtaNode>("OtaNode");

    factory.registerScriptingEnums<OtaState>();

    auto tree = factory.createTreeFromText(xml_tree);

    auto blackboard = tree.rootBlackboard();

    int  i      = 0;
    auto status = tree.tickOnce();
    UTILS_LOG_INFO("OtaNode status: {}", BT::toStr(status));
    while (status == BT::NodeStatus::RUNNING) {
        i++;
        if (i >= 100) {
            // 设置 ota_state 为 COMPLETED
            blackboard->set("ota_state", OtaState::COMPLETED);
        }

        UTILS_LOG_INFO("OtaNode i: {}", i);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        status = tree.tickOnce();
        UTILS_LOG_INFO("OtaNode status: {}", BT::toStr(status));
    }
    UTILS_LOG_INFO("OtaNode status: {}", BT::toStr(status));

    return 0;
}
