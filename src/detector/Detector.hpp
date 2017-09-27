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
    Detector(int header_length);
    
    SwitchId addSwitch(SwitchId id, std::vector<PortId> port_list);
    void deleteSwtich(SwitchId id);
    
    TableId addTable(SwitchId switch_id, TableId table_id);
    void deleteTable(SwitchId switch_id, TableId table_id);
    
    RuleInfo addRule(SwitchId switch_id, TableId table_id,
                     uint16_t priority, Match& match,
                     std::vector<Action>& action_list);
    void deleteRule(RuleInfo rule_info);
    
    void addLink(SwitchId src_switch_id, PortId src_port_id,
                 SwitchId dst_switch_id, PortId dst_port_id);
    void deleteLink(SwitchId src_switch_id, PortId src_port_id,
                    SwitchId dst_switch_id, PortId dst_port_id);

private:
    DependencyGraph dependency_graph_;
    FlowPredictor flow_predictor_;

};
