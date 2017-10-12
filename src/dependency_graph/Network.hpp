#pragma once

#include "../Common.hpp"
#include "Rule.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <vector>

class Switch;
class Table;

typedef std::shared_ptr<Switch> SwitchPtr;
typedef std::shared_ptr<Table> TablePtr;

class Table
{
public:
    Table(SwitchId switch_id, TableId id);
    
    RulePtr getRule(RuleId id);
    RulePtr addRule(RuleId rule_id, uint16_t priority,
                    NetworkSpace& match,
                    std::vector<Action>& action_list);
    void deleteRule(RuleId id);
    
    RuleRange rules() const {return RuleRange(sorted_rule_map_);}
    RuleRange upperRules(RulePtr rule) const;
    RuleRange lowerRules(RulePtr rule) const;
    
    inline SwitchId switchId() const {return switch_id_;}
    inline TableId id() const {return id_;}

private:
    SwitchId switch_id_;
    TableId id_;
    
    std::map<RuleId, RulePtr> rule_map_;
    
    // Experimental
    // These maps are used in order to have a list of rules
    // that is already sorted by priority, and to have a map
    // from RuleId to rule pointers
    using PriorityMap   = std::map<RuleId, Priority>;
    using RuleMap       = std::map<RuleId, RulePtr>;
    using SortedRuleMap = std::map<Priority, RuleMap>;
    
    PriorityMap priority_map_;
    SortedRuleMap sorted_rule_map_
    
    RuleMap* get_rule_map(Priority priority);

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
    
    std::vector<PortId>& ports();
    inline SwitchId id() const {return id_;}

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
                    NetworkSpace& match,
                    std::vector<Action>& action_list);
    void deleteRule(SwitchId switch_id,
                    TableId table_id,
                    RuleId rule_id);
    
    static bool isSpecialPort(PortId id);

private:
    std::map<SwitchId, SwitchPtr> switch_map_;

};
