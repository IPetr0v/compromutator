#pragma once

#include "../Common.hpp"

#include <algorithm>
#include <map>
#include <vector>

class Rule
{
public:
    Rule(SwitchId switch_id, TableId table_id, RuleId id,
         uint16_t priority, Match& match,
         std::vector<Action>& action_list);
    
    inline const SwitchId switchId() {return switch_id_;}
    inline const TableId tableId() {return table_id_;}
    inline const RuleId id() {return id_;}
    
    inline const uint16_t priority() {return priority_;}
    inline const Match& match() {return match_;}
    inline const std::vector<Action>& action_list() {return action_list_;}
    
    inline const PortId& inPort() {match_.in_port;}
    inline const std::vector<PortId>& outPorts() {return out_ports_;}
    
private:
    SwitchId switch_id_;
    TableId table_id_;
    RuleId id_;
    
    uint16_t priority_;
    Match match_;
    std::vector<Action> action_list_;
    
    std::vector<PortId> out_ports_;

};

class Table
{
public:
    Table(SwitchId switch_id, TableId id);
    
    RulePtr getRule(RuleId id);
    RulePtr addRule(RuleId rule_id, uint16_t priority, Match& match,
                    std::vector<Action>& action_list);
    void deleteRule(RuleId id);
    
    inline const SwitchId switchId() {return switch_id_;}
    inline const TableId id() {return id_;}

private:
    SwitchId switch_id_;
    TableId id_;
    std::map<RuleId, RulePtr> rule_map_;

};

class Switch
{
public:
    Switch(SwitchId id, std::vector<PortId>& port_list);
    
    PortId addPort(PortId id);
    void deletePort(PortId id);
    
    TablePtr getTable(TableId id);
    TablePtr getFrontTable();
    TablePtr addTable(TableId table_id);
    void deleteTable(TableId id);
    
    std::vector<PortId> ports();
    inline const SwitchId id() {return id_;}

private:
    SwitchId id_;
    std::vector<PortId> ports_;
    std::map<TableId, TablePtr> table_map_;

};

class Network
{
public:
    Network();
    
    // Switch management
    SwitchPtr getSwitch(SwitchId id);
    SwitchPtr addSwitch(SwitchId id, std::vector<PortId>& port_list);
    void deleteSwitch(SwitchId id);
    
    // Table management
    TablePtr getTable(SwitchId switch_id, TableId table_id);
    TablePtr addTable(SwitchId switch_id, TableId table_id);
    void deleteTable(SwitchId switch_id, TableId table_id);
    
    // Rule management
    RulePtr getRule(SwitchId switch_id,
                    TableId table_id,
                    RuleId rule_id);
    RulePtr addRule(SwitchId switch_id, TableId table_id,
                    RuleId rule_id, uint16_t priority,
                    Match& match, std::vector<Action>& action_list);
    void deleteRule(SwitchId switch_id,
                    TableId table_id,
                    RuleId rule_id);
    
    static bool isReservedPort(PortId id);

private:
    std::map<SwitchId, SwitchPtr> switch_map_;

};
