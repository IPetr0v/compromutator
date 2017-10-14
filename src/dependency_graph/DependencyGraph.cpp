#include "DependencyGraph.hpp"

DependencyGraph::DependencyGraph():
    topology_(network_),
    dependency_updater_(network_, topology_)
{
    
}

SwitchId DependencyGraph::addSwitch(SwitchId id, std::vector<PortId>& port_list)
{
    SwitchPtr sw = network_.addSwitch(id, port_list);
    return sw ? sw->id() : (SwitchId)0;
}

void DependencyGraph::deleteSwitch(SwitchId id)
{
    network_.deleteSwitch(id);
}

TableId DependencyGraph::addTable(SwitchId switch_id, TableId table_id)
{
    TablePtr table = network_.addTable(switch_id, table_id);
    return table ? table->id() : (TableId)0;
}

void DependencyGraph::deleteTable(SwitchId switch_id, TableId table_id)
{
    network_.deleteTable(switch_id, table_id);
}

RulePtr DependencyGraph::addRule(SwitchId switch_id, TableId table_id,
                                 uint16_t priority,NetworkSpace& match,
                                 std::vector<Action>& action_list)
{
    // Create rule data
    RulePtr rule = network_.addRule(switch_id, table_id, priority,
                                    match, action_list);
    
    // Create port dependencies
    topology_.addRule(rule);
    
    // Add rule to the dependency updater
    dependency_updater_.addRule(rule);
    
    // TODO: create error codes for RuleInfo
    return rule ? rule : nullptr;
}

void DependencyGraph::deleteRule(SwitchId switch_id, TableId table_id,
                                 RuleId rule_id)
{
    network_.deleteRule(switch_id,
                        table_id,
                        rule_id);
}

void DependencyGraph::addLink(SwitchId src_switch_id, PortId src_port_id,
                              SwitchId dst_switch_id, PortId dst_port_id)
{
    SwitchPtr src_switch = network_.getSwitch(src_switch_id);
    SwitchPtr dst_switch = network_.getSwitch(dst_switch_id);

    // Create link
    bool link_added = topology_.addLink(src_switch_id, src_port_id,
                                        dst_switch_id, dst_port_id);
    
    // Create new dependencies
    if (link_added) {
        dependency_updater_.addLink(src_switch_id, src_port_id,
                                    dst_switch_id, dst_port_id);
    }
}

void DependencyGraph::deleteLink(SwitchId src_switch_id, PortId src_port_id,
                                 SwitchId dst_switch_id, PortId dst_port_id)
{
    
}
