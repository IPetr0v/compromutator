#include "DependencyGraph.hpp"

DependencyGraph::DependencyGraph():
    topology_(network_), dependency_updater_(network_, topology_)
{
    
}

SwitchId DependencyGraph::addSwitch(SwitchId id, std::vector<PortId>& port_list)
{
    SwitchPtr sw = network_.addSwitch(id, port_list);
    if (sw) {
        // TODO: check if const shared_ptr& is ok!!!
        // Create table miss rules dependencies
        for (const auto& table : sw->tables()) {
            RulePtr table_miss_rule = table->tableMissRule();
            topology_.addRule(table_miss_rule);
            dependency_updater_.addRule(table_miss_rule);
        }

        return sw->id();
    }
    else {
        return (SwitchId)0;
    }
}

void DependencyGraph::deleteSwitch(SwitchId id)
{
    // TODO: delete table miss and source/sink rules
    network_.deleteSwitch(id);
}

TableId DependencyGraph::addTable(SwitchId switch_id, TableId table_id)
{
    TablePtr table = network_.addTable(switch_id, table_id);
    if (table) {
        // Create table miss rule dependencies
        RulePtr table_miss_rule = table->tableMissRule();
        topology_.addRule(table_miss_rule);
        dependency_updater_.addRule(table_miss_rule);

        return table->id();
    }
    else {
        return (TableId)0;
    }
}

void DependencyGraph::deleteTable(SwitchId switch_id, TableId table_id)
{
    // TODO: delete table miss rule
    network_.deleteTable(switch_id, table_id);
}

RulePtr DependencyGraph::addRule(SwitchId switch_id, TableId table_id,
                                 uint16_t priority,NetworkSpace& match,
                                 std::vector<Action>& action_list)
{
    // Create rule data
    RulePtr rule = network_.addRule(switch_id, table_id, priority,
                                    match, action_list);
    
    // Create port and rule dependencies
    topology_.addRule(rule);
    dependency_updater_.addRule(rule);
    
    // TODO: create error codes for RuleInfo
    return rule ? rule : nullptr;
}

void DependencyGraph::deleteRule(SwitchId switch_id, TableId table_id,
                                 RuleId rule_id)
{
    network_.deleteRule(switch_id, table_id, rule_id);
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

RulePtr DependencyGraph::dropRule()
{
    return network_.dropRule();
}

RulePtr DependencyGraph::controllerRule()
{
    return network_.controllerRule();
}

RulePtr DependencyGraph::sourceRule(SwitchId switch_id, PortId port_id)
{
    return network_.sourceRule(switch_id, port_id);
}

RulePtr DependencyGraph::sinkRule(SwitchId switch_id, PortId port_id)
{
    return network_.sinkRule(switch_id, port_id);
}

RulePtr DependencyGraph::tableMissRule(SwitchId switch_id, TableId table_id)
{
    return network_.tableMissRule(switch_id, table_id);
}

DependencyPtr DependencyGraph::getDependency(RuleId src_rule_id,
                                             RuleId dst_rule_id)
{
    return dependency_updater_.getDependency(src_rule_id, dst_rule_id);
}

std::list<DependencyPtr> DependencyGraph::inDependencies(RulePtr rule)
{
    return dependency_updater_.outDependencies(rule);
}

std::list<DependencyPtr> DependencyGraph::outDependencies(RulePtr rule)
{
    return dependency_updater_.outDependencies(rule);
}

std::map<std::pair<RuleId, RuleId>, DependencyPtr>
DependencyGraph::dependencies()
{
    return dependency_updater_.dependencies();
}
