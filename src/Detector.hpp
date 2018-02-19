#pragma once

#include "ConcurrencyPrimitives.hpp"
#include "Controller.hpp"
#include "network/Network.hpp"
#include "network/DependencyGraph.hpp"
#include "flow_predictor/FlowPredictor.hpp"

#include <memory>

struct SwitchInfo
{
    SwitchId id;
    std::vector<PortId> ports;
    uint8_t table_number;
};

using InstructionQueue = ConcurrentAlarmingQueue<Instruction>;

class Detector
{
public:
    explicit Detector(std::shared_ptr<Alarm> alarm);
    ~Detector();

    // TODO: make soft instruction request

    bool instructionsExist() {
        return not instruction_queue_.empty();
    }

    Instruction getInstruction() {
        return instruction_queue_.pop();
    }

    void addSwitch(SwitchInfo info);
    void deleteSwitch(SwitchId id);

    void addRule(SwitchId switch_id, RuleInfo info);
    void deleteRule(SwitchId switch_id, RuleInfo info);

    void addLink(TopoId src_topo_id, TopoId dst_topo_id);
    void deleteLink(TopoId src_topo_id, TopoId dst_topo_id);

    void addRuleStats(RequestId xid, RuleStatsFields stats);
    void addPortStats(RequestId xid, PortStatsFields stats);

    class Impl;

private:
    InstructionQueue instruction_queue_;
    std::unique_ptr<Impl> impl_;

    // Detector runs in a separate thread
    Executor executor_;

};
