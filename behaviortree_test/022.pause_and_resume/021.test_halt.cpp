#include <deque>

#include "behaviortree_cpp/bt_factory.h"
#include "behaviortree_cpp/decorators/loop_node.h"
#include "behaviortree_cpp/loggers/bt_cout_logger.h"

using namespace BT;

/*
 * In this example we will show how a common design pattern could be implemented.
 * We want to iterate through the elements of a queue, for instance a list of waypoints.
 */

struct Pose2D {
    double x, y, theta;
};

/**
 * @brief Dummy action that generates a list of poses.
 */
class GenerateWaypoints : public StatefulActionNode {
   public:
    GenerateWaypoints(const std::string& name, const NodeConfig& config)
        : StatefulActionNode(name, config) {}

    NodeStatus onStart() override {
        std::cout << "GenerateWaypoints started" << std::endl;
        return NodeStatus::RUNNING;
    }

    NodeStatus onRunning() override {
        std::cout << "GenerateWaypoints running" << std::endl;
        return NodeStatus::RUNNING;
    }

    void onHalted() override { std::cout << "GenerateWaypoints halted" << std::endl; }

    static PortsList providedPorts() { return {}; }
};
//--------------------------------------------------------------
class PrintNumber : public SyncActionNode {
   public:
    PrintNumber(const std::string& name, const NodeConfig& config) : SyncActionNode(name, config) {}
    NodeStatus tick() override {
        if (is_first_time_) {
            is_first_time_ = false;
            return NodeStatus::FAILURE;
        } else {
            return NodeStatus::SUCCESS;
        }
    }

    static PortsList providedPorts() { return {}; }

   private:
    int is_first_time_ = true;
};

//--------------------------------------------------------------

// clang-format off
static const char* xml_tree = R"(
 <root BTCPP_format="4" >
     <BehaviorTree ID="MainTree">
        <ReactiveFallback>
            <PrintNumber />
            <GenerateWaypoints />
        </ReactiveFallback>
     </BehaviorTree>
 </root>
 )";

// clang-format on

int main() {
    BehaviorTreeFactory factory;

    factory.registerNodeType<PrintNumber>("PrintNumber");
    factory.registerNodeType<GenerateWaypoints>("GenerateWaypoints");

    factory.registerBehaviorTreeFromText(xml_tree);
    auto tree = factory.createTree("MainTree");

    auto status = tree.tickOnce();
    while (status == NodeStatus::RUNNING) {
        status = tree.tickOnce();
    }
    std::cout << "Status: " << toStr(status) << std::endl;

    return 0;
}
