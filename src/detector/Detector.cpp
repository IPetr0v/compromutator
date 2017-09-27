#include "Detector.hpp"

Detector::Detector(int header_length):
    dependency_graph_(header_length),
    flow_predictor_(dependency_graph_)
{
    
}

SwitchId Detector::addSwitch(SwitchId id, std::vector<PortId> port_list)
{
    return dependency_graph_.addSwitch(id, port_list);
}

void Detector::deleteSwtich(SwitchId id)
{
    dependency_graph_.deleteSwitch(id);
}

TableId Detector::addTable(SwitchId switch_id, TableId table_id)
{
    return dependency_graph_.addTable(switch_id, table_id);
}

void Detector::deleteTable(SwitchId switch_id, TableId table_id)
{
    dependency_graph_.deleteTable(switch_id, table_id);
}

RuleInfo Detector::addRule(SwitchId switch_id, TableId table_id,
                         uint16_t priority, Match& match,
                         std::vector<Action>& action_list)
{
    // Generate rule id
    RuleId rule_id = 0xFF;
    RuleInfo rule_info{switch_id, table_id, rule_id};
    
    // Create rule data
    RulePtr rule = dependency_graph_.addRule(switch_id, table_id,
                                             rule_id, priority,
                                             match, action_list);
    
    // Insert rule data in the flow predictor
    flow_predictor_.addRule(rule);
    
    // TODO: create error codes for RuleInfo
    return rule ? rule_info : RuleInfo{0, 0, 0};
}

void Detector::deleteRule(RuleInfo rule_info)
{
    dependency_graph_.deleteRule(rule_info.switch_id,
                                 rule_info.table_id,
                                 rule_info.rule_id);
}

void Detector::addLink(SwitchId src_switch_id, PortId src_port_id,
                       SwitchId dst_switch_id, PortId dst_port_id)
{
    
}

void Detector::deleteLink(SwitchId src_switch_id, PortId src_port_id,
                          SwitchId dst_switch_id, PortId dst_port_id)
{
    
}
