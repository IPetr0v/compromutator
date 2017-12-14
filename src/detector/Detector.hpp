#pragma once

#include "../Common.hpp"
#include "../dependency_graph/DependencyGraph.hpp"
#include "FlowPredictor.hpp"

struct RuleInfo
{
    SwitchId switch_id;
    TableId table_id;
    RuleId rule_id;
};

class Detector
{
public:
    Detector();

    InterceptorDiff addSwitch(SwitchId switch_id, std::vector<PortId> ports);
    InterceptorDiff deleteSwitch(SwitchId id);

    InterceptorDiff addTable(SwitchId switch_id, TableId table_id);
    InterceptorDiff deleteTable(SwitchId switch_id, TableId table_id);

    InterceptorDiff addRule(SwitchId switch_id, TableId table_id,
                            uint16_t priority, NetworkSpace& domain,
                            std::vector<Action>& actions);
    InterceptorDiff deleteRule(RuleInfo rule_info);
    
    void addLink(SwitchId src_switch_id, PortId src_port_id,
                 SwitchId dst_switch_id, PortId dst_port_id);
    void deleteLink(SwitchId src_switch_id, PortId src_port_id,
                    SwitchId dst_switch_id, PortId dst_port_id);

private:
    DependencyGraph dependency_graph_;
    FlowPredictor flow_predictor_;

};
