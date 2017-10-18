#pragma once

#include "../Common.hpp"
#include "../NetworkSpace.hpp"
#include "Network.hpp"
#include "Topology.hpp"

#include <algorithm>
#include <vector>

class DependencyUpdater
{
public:
    DependencyUpdater(Network& network, Topology& topology);
    
    RulePtr addRule(RulePtr rule);
    void deleteRule(RulePtr old_rule);
    
    void addLink(SwitchId src_switch_id, PortId src_port_id,
                 SwitchId dst_switch_id, PortId dst_port_id);
    void deleteLink(SwitchId src_switch_id, PortId src_port_id,
                    SwitchId dst_switch_id, PortId dst_port_id);

    std::map<std::pair<RuleId, RuleId>, DependencyPtr> dependencies();

private:
    Network& network_;
    Topology& topology_;

    // TODO: add SOURCE and TARGET rules
    
    DependencyPtr add_table_dependency(RulePtr src_rule, RulePtr dst_rule,
                                       NetworkSpace domain);
    DependencyPtr add_dependency(RulePtr src_rule, RulePtr dst_rule);
    DependencyPtr add_dependency(RulePtr src_rule, RulePtr dst_rule,
                                 NetworkSpace domain);
    void delete_dependency(DependencyPtr dependency);
    
    void add_dependencies(RulePtr src_rule, SwitchId src_switch_id,
                          PortId src_port_id);
    void add_dependencies(RulePtr src_rule, TableId dst_table_id);
    void add_dependencies(RulePtr src_rule, std::vector<RulePtr>& dst_rules);
    void add_dependencies(RulePtr src_rule, RuleRange& dst_rules);
    
    void add_table_dependencies(RulePtr new_rule);
    void delete_table_dependencies(RulePtr old_rule);
    
    void add_out_dependencies(RulePtr new_rule);
    void delete_out_dependencies(RulePtr old_rule);

};
