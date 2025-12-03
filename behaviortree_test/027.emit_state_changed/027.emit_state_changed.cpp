#include <behaviortree_cpp/bt_factory.h>
#include <behaviortree_cpp/loggers/bt_cout_logger.h>
#include <thread>
#include <chrono>

using namespace BT;

// 一个简单节点：从黑板读取值
class PrintNumber : public SyncActionNode
{
public:
    PrintNumber(const std::string& name, const NodeConfig& config)
        : SyncActionNode(name, config) {}

    static PortsList providedPorts()
    {
        return { InputPort<int>("number") };
    }

    NodeStatus tick() override
    {
        auto val = getInput<int>("number");
        if (!val)
        {
            std::cout << "[PrintNumber] No number" << std::endl;
            return NodeStatus::FAILURE;
        }

        std::cout << "[PrintNumber] number = " << val.value() << std::endl;
        return NodeStatus::SUCCESS;
    }
};

// XML：只有一个节点 PrintNumber
static const char* xml_text = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="MainTree">
    <Sequence>
        <PrintNumber number="{the_number}"/>
    </Sequence>
  </BehaviorTree>
</root>
)";

int main()
{
    BehaviorTreeFactory factory;
    factory.registerNodeType<PrintNumber>("PrintNumber");

    auto tree = factory.createTreeFromText(xml_text);

    // 日志输出
    StdCoutLogger logger(tree);

    // 初始黑板值
    tree.rootBlackboard()->set("the_number", 0);

    // ====== 在后台线程 2 秒后唤醒 Tree ======
    std::thread event_thread([&tree]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "\n[ExternalEvent] External event occurred!\n";

        // 修改黑板（可选，只是为了演示）
        tree.rootBlackboard()->set("the_number", 42);

        // 关键：调用 emitWakeUpSignal() 来唤醒 Tree::sleep()
        // 注意：
        // 1. 修改 blackboard 不会自动触发 emitWakeUpSignal，需要手动调用
        // 2. 只要调用 emitWakeUpSignal() 就能唤醒 Tree，不一定需要修改 blackboard
        // 3. emitWakeUpSignal() 和修改 blackboard 是完全独立的两个操作
        std::cout << "[ExternalEvent] Calling emitWakeUpSignal() to wake up the tree\n";
        tree.emitWakeUpSignal();
    });

    tree.tickOnce();

    // tree.sleep() 会等待直到：
    // 1. 超时（5秒后）
    // 2. 或者被 emitWakeUpSignal() 唤醒（提前返回）
    std::cout << "[Tick] Tree is sleeping...\n";
    auto current_time = std::chrono::steady_clock::now();
    bool was_woken = tree.sleep(std::chrono::milliseconds(5000));  // 本来应该睡 5 秒
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - current_time);
    std::cout << "sleep time: " << duration.count() << "ms" << std::endl;

    if (was_woken) {
        std::cout << "[Wakeup] Tree woken by emitWakeUpSignal() (before timeout)\n";
    } else {
        std::cout << "[Wakeup] Tree woken by timeout\n";
    }

    // 再次 tick，会看到新值
    tree.tickOnce();

    event_thread.join();
    return 0;
}
