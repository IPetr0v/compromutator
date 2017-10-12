#pragma once

#include "../Common.hpp"
#include "Network.hpp"
#include "Topology.hpp"

class DependencyGraph
{
public:
    DependencyGraph(int header_length);
    
    SwitchId addSwitch(SwitchId id, std::vector<PortId>& port_list);
    void deleteSwitch(SwitchId id);
    
    TableId addTable(SwitchId switch_id, TableId table_id);
    void deleteTable(SwitchId switch_id, TableId table_id);
    
    RulePtr addRule(SwitchId switch_id, TableId table_id,
                    RuleId rule_id, uint16_t priority,
                    NetworkSpace& match,
                    std::vector<Action>& action_list);
    void deleteRule(SwitchId switch_id, TableId table_id, RuleId rule_id);
    
    void addLink(SwitchId src_switch_id, PortId src_port_id,
                 SwitchId dst_switch_id, PortId dst_port_id);
    void deleteLink(SwitchId src_switch_id, PortId src_port_id,
                    SwitchId dst_switch_id, PortId dst_port_id);

private:
    Network network_;
    Topology topology_;
    DependencyUpdater dependency_updater_;

};
