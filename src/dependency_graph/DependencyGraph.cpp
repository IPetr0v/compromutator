#include "DependencyGraph.hpp"

DependencyGraph::DependencyGraph(int header_length)
{
    
}

SwitchId DependencyGraph::addSwitch(SwitchId id, std::vector<PortId>& port_list)
{
    SwitchPtr sw = network_.addSwitch(id, port_list);
    return sw ? sw->id() : 0;
}

void DependencyGraph::deleteSwitch(SwitchId id)
{
    network_.deleteSwitch(id);
}

TableId DependencyGraph::addTable(SwitchId switch_id, TableId table_id)
{
    TablePtr table = network_.addTable(switch_id, table_id);
    return table ? table->id() : 0;
}

void DependencyGraph::deleteTable(SwitchId switch_id, TableId table_id)
{
    network_.deleteTable(switch_id, table_id);
}

RulePtr DependencyGraph::addRule(SwitchId switch_id, TableId table_id,
                                 RuleId rule_id, uint16_t priority,
                                 Match& match, std::vector<Action>& action_list)
{
    // Create rule data
    RulePtr rule = network_.addRule(switch_id, table_id, rule_id,
                                    priority, match, action_list);
    
    // Create rule vertex
    
    // TODO: create error codes for RuleInfo
    return rule ? rule : nullptr;
}

void DependencyGraph::deleteRule(SwitchId switch_id, TableId table_id, RuleId rule_id)
{
    network_.deleteRule(switch_id,
                        table_id,
                        rule_id);
}

void DependencyGraph::addLink(SwitchId src_switch_id, PortId src_port_id,
                       SwitchId dst_switch_id, PortId dst_port_id)
{
    /*bool link_added;
    SwitchPtr src_switch = network_.getSwitch(src_switch_id);
    SwitchPtr dst_switch = network_.getSwitch(dst_switch_id);
    link_added = topology_.addLink(src_switch, src_port_id,
                                   dst_switch, dst_port_id);
    
    // Check dependancy graph for new arcs
    if (link_added) {
        addLinkDependency(src_switch_id, src_port_id,
                          dst_switch_id, dst_port_id);
    }*/
}

void DependencyGraph::deleteLink(SwitchId src_switch_id, PortId src_port_id,
                          SwitchId dst_switch_id, PortId dst_port_id)
{
    
}
