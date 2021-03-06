#pragma once

#include "ConcurrencyPrimitives.hpp"
#include "PerformanceMonitor.hpp"
#include "network/Network.hpp"
#include "network/DependencyGraph.hpp"
#include "flow_predictor/FlowPredictor.hpp"

#include <memory>

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

    size_t instructionNumber() {
        return instruction_queue_.size();
    }

    Instruction getInstruction() {
        return instruction_queue_.pop();
    }

    void fillMeasurement(PerformanceMeasurementPtr measurement);

    void addSwitch(SwitchInfo info);
    void deleteSwitch(SwitchId id);

    void addRule(RuleInfo info);
    void changeRule(RuleInfo info);
    void deleteRule(RuleInfo info);

    void addLink(TopoId src_topo_id, TopoId dst_topo_id);
    void deleteLink(TopoId src_topo_id, TopoId dst_topo_id);

    void getRuleStats(RequestId request_id, RuleInfo info);
    void getPortStats(RequestId request_id, PortInfo info);

    // TODO: do we need to check match, or we can only check xid ?
    void addRuleStats(RequestId request_id, RuleInfo info,
                      RuleStatsFields stats);
    void addPortStats(RequestId request_id, PortInfo info,
                      PortStatsFields stats);

    void prepareInstructions();

    class Impl;

private:
    InstructionQueue instruction_queue_;
    std::unique_ptr<Impl> impl_;

    // Detector runs in a separate thread
    Executor executor_;

};
