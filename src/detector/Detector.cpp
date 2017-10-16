#include "Detector.hpp"

Detector::Detector():
    flow_predictor_(dependency_graph_)
{
    
}

SwitchId Detector::addSwitch(SwitchId id, std::vector<PortId> port_list)
{
    return dependency_graph_.addSwitch(id, port_list);
}

void Detector::deleteSwitch(SwitchId id)
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
                           uint16_t priority, NetworkSpace& domain,
                           std::vector<Action>& action_list)
{
    // Create rule data
    RulePtr rule = dependency_graph_.addRule(switch_id, table_id, priority,
                                             domain, action_list);
    
    // Insert rule data in the flow predictor
    flow_predictor_.addRule(rule);
    
    // TODO: create error codes for RuleInfo
    return rule ? RuleInfo{switch_id, table_id, rule->id()}
                : RuleInfo{0, 0, 0};
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
    dependency_graph_.addLink(src_switch_id, src_port_id,
                              dst_switch_id, dst_port_id);
}

void Detector::deleteLink(SwitchId src_switch_id, PortId src_port_id,
                          SwitchId dst_switch_id, PortId dst_port_id)
{
    
}
