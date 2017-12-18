#include "Detector.hpp"

Detector::Detector():
    flow_predictor_(dependency_graph_)
{
    /*flow_predictor_.addRule(dependency_graph_.dropRule());
    flow_predictor_.addRule(dependency_graph_.controllerRule());*/
}

InterceptorDiff Detector::addSwitch(SwitchId switch_id,
                                    std::vector<PortId> ports)
{
    dependency_graph_.addSwitch(switch_id, ports);

    auto diff = dependency_graph_.getLatestDiff();
    return flow_predictor_.update(diff);
}

InterceptorDiff Detector::deleteSwitch(SwitchId id)
{
    dependency_graph_.deleteSwitch(id);

    auto diff = dependency_graph_.getLatestDiff();
    return flow_predictor_.update(diff);
}

InterceptorDiff Detector::addTable(SwitchId switch_id, TableId table_id)
{
    dependency_graph_.addTable(switch_id, table_id);

    // Add special rule to the flow predictor
    //auto table_miss_rule = dependency_graph_.tableMissRule(switch_id, table_id);
    auto diff = dependency_graph_.getLatestDiff();
    return flow_predictor_.update(diff);
}

InterceptorDiff Detector::deleteTable(SwitchId switch_id, TableId table_id)
{
    dependency_graph_.deleteTable(switch_id, table_id);

    auto diff = dependency_graph_.getLatestDiff();
    return flow_predictor_.update(diff);
}

InterceptorDiff Detector::addRule(SwitchId switch_id, TableId table_id,
                                  uint16_t priority, NetworkSpace& domain,
                                  std::vector<Action>& actions)
{
    // Create rule data
    auto rule = dependency_graph_.addRule(switch_id, table_id,
                                          priority, domain, actions);
    
    // Insert rule in the flow predictor
    auto diff = dependency_graph_.getLatestDiff();
    return flow_predictor_.update(diff);
}

InterceptorDiff Detector::deleteRule(RuleInfo rule_info)
{
    dependency_graph_.deleteRule(rule_info.switch_id,
                                 rule_info.table_id,
                                 rule_info.rule_id);
    auto diff = dependency_graph_.getLatestDiff();
    return flow_predictor_.update(diff);
}

InterceptorDiff Detector::addLink(SwitchId src_switch_id, PortId src_port_id,
                                  SwitchId dst_switch_id, PortId dst_port_id)
{
    dependency_graph_.addLink(src_switch_id, src_port_id,
                              dst_switch_id, dst_port_id);
    auto diff = dependency_graph_.getLatestDiff();
    return flow_predictor_.update(diff);
}

InterceptorDiff Detector::deleteLink(SwitchId src_switch_id, PortId src_port_id,
                                     SwitchId dst_switch_id, PortId dst_port_id)
{
    dependency_graph_.deleteLink(src_switch_id, src_port_id,
                                 dst_switch_id, dst_port_id);
    auto diff = dependency_graph_.getLatestDiff();
    return flow_predictor_.update(diff);
}
