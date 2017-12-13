#include "Detector.hpp"

Detector::Detector():
    flow_predictor_(dependency_graph_)
{
    flow_predictor_.addRule(dependency_graph_.dropRule());
    flow_predictor_.addRule(dependency_graph_.controllerRule());
}

SwitchId Detector::addSwitch(SwitchId switch_id, std::vector<PortId> ports)
{
    dependency_graph_.addSwitch(switch_id, ports);

    // Add special rules to the flow predictor
    for (auto port_id : ports) {
        auto src_rule = dependency_graph_.sourceRule(switch_id, port_id);
        auto dst_rule = dependency_graph_.sinkRule(switch_id, port_id);
        flow_predictor_.addRule(src_rule);
        flow_predictor_.addRule(dst_rule);
    }
    auto table_miss_rule = dependency_graph_.tableMissRule(switch_id, 0);
    flow_predictor_.addRule(table_miss_rule);

    return switch_id;
}

void Detector::deleteSwitch(SwitchId id)
{
    dependency_graph_.deleteSwitch(id);
}

TableId Detector::addTable(SwitchId switch_id, TableId table_id)
{
    dependency_graph_.addTable(switch_id, table_id);

    // Add special rule to the flow predictor
    auto table_miss_rule = dependency_graph_.tableMissRule(switch_id, table_id);
    flow_predictor_.addRule(table_miss_rule);

    return table_id;
}

void Detector::deleteTable(SwitchId switch_id, TableId table_id)
{
    dependency_graph_.deleteTable(switch_id, table_id);
}

RuleInfo Detector::addRule(SwitchId switch_id, TableId table_id,
                           uint16_t priority, NetworkSpace& domain,
                           std::vector<Action>& actions)
{
    // Create rule data
    auto rule = dependency_graph_.addRule(switch_id, table_id,
                                          priority, domain, actions);
    
    // Insert rule in the flow predictor
    flow_predictor_.addRule(rule);
    
    // TODO: create error codes for RuleInfo
    return rule ? RuleInfo{switch_id, table_id, rule->id()} : RuleInfo{0, 0, 0};
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
