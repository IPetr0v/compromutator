#pragma once

#include "../Common.hpp"
#include "Network.hpp"
#include "Topology.hpp"

class DependencyUpdater
{
public:
    DependencyUpdater(int header_length):
        header_length_(header_length) {}
    
    /*SwitchId addSwitch(SwitchId id, std::vector<PortNum>& port_list);
    void deleteSwtich(SwitchId id);
    
    TableId addTable(SwitchId switch_id, TableId table_id);
    void deleteTable(SwitchId switch_id, TableId table_id);*/
    
    RulePtr addRule(RulePtr rule);
    void deleteRule(RulePtr old_rule);
    
    void addLink(SwitchId src_switch_id, PortId src_port_id,
                 SwitchId dst_switch_id, PortId dst_port_id);
    void deleteLink(SwitchId src_switch_id, PortId src_port_id,
                    SwitchId dst_switch_id, PortId dst_port_id);

private:
    int header_length_;
    
    Network& network_;
    Topology& topology_;
    
    // Special rules
    RulePtr drop_;
    RulePtr controller_;
    // TODO: add SOURCE and TARGET rules
    
    DependencyPtr add_table_dependency(RulePtr src_rule, RulePtr dst_rule,
                                       NetworkSpace domain);
    DependencyPtr add_dependency(RulePtr src_rule, RulePtr dst_rule,
                                 NetworkSpace domain)
    void add_dependencies(RulePtr src_rule, SwtichId dst_switch_id,
                          PortId dst_port_id);
    void add_dependencies(RulePtr src_rule, TableId dst_table_id);
    void add_dependencies(RulePtr src_rule, std::vector<RulePtr>& dst_rules);
    void add_dependencies(RulePtr src_rule, RuleRange& dst_rules);
    void add_table_dependencies(RulePtr new_rule);
    void delete_table_dependencies(RulePtr old_rule);
    void add_out_dependencies(RulePtr new_rule);
    void delete_out_dependencies(RulePtr old_rule);

};
