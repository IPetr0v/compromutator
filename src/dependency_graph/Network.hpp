#pragma once

#include "../Common.hpp"
#include "Rule.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <vector>

class Switch;
class Port;
class Table;

typedef std::shared_ptr<Switch> SwitchPtr;
typedef std::shared_ptr<Port> PortPtr;
typedef std::shared_ptr<Table> TablePtr;

class Table
{
public:
    Table(SwitchId switch_id, TableId id);

    RulePtr getRule(RuleId id);
    RulePtr addRule(uint16_t priority, NetworkSpace& domain,
                    std::vector<Action>& action_list);
    void deleteRule(RuleId id);

    RulePtr tableMissRule() {return table_miss_rule_;}
    RuleRange rules() {return RuleRange(sorted_rule_map_);}
    RuleRange upperRules(RulePtr rule);
    RuleRange lowerRules(RulePtr rule);
    
    SwitchId switchId() const {return switch_id_;}
    TableId id() const {return id_;}

private:
    SwitchId switch_id_;
    TableId id_;

    RulePtr table_miss_rule_;

    // These maps are used in order to have a list of rules
    // that is already sorted by priority, and to have a map
    // from RuleId to rule pointers
    PriorityMap priority_map_;
    SortedRuleMap sorted_rule_map_;

};

class Port
{
public:
    Port(SwitchId switch_id, PortId id);

    RulePtr sourceRule() const {return source_rule_;}
    RulePtr sinkRule() const {return sink_rule_;}

    SwitchId switchId() const {return switch_id_;}
    PortId id() const {return id_;}

private:
    SwitchId switch_id_;
    PortId id_;

    RulePtr source_rule_;
    RulePtr sink_rule_;
};

class Switch
{
public:
    Switch(SwitchId id, std::vector<PortId>& port_list);

    PortPtr getPort(PortId port_id);
    PortPtr addPort(PortId port_id);
    void deletePort(PortId id);

    TablePtr getTable(TableId id);
    TablePtr addTable(TableId table_id);
    void deleteTable(TableId id);

    TablePtr frontTable() const {return front_table_;}

    // TODO: maybe make it const, but how to optimally return ports?
    std::vector<PortId>& portIdList();
    std::vector<PortPtr> ports();
    std::vector<TablePtr> tables();
    SwitchId id() const {return id_;}

private:
    SwitchId id_;
    std::vector<PortId> ports_;
    std::map<PortId, PortPtr> port_map_;
    std::map<TableId, TablePtr> table_map_;

    TablePtr front_table_;

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
    RulePtr getRule(RuleId rule_id);
    RulePtr getRule(SwitchId switch_id, TableId table_id, RuleId rule_id);
    RulePtr addRule(SwitchId switch_id, TableId table_id,
                    uint16_t priority, NetworkSpace& domain,
                    std::vector<Action>& action_list);
    void deleteRule(SwitchId switch_id, TableId table_id, RuleId rule_id);
    RulePtr dropRule() {return drop_rule_;}
    RulePtr controllerRule() {return controller_rule_;}
    RulePtr sourceRule(SwitchId switch_id, PortId port_id);
    RulePtr sinkRule(SwitchId switch_id, PortId port_id);
    RulePtr tableMissRule(SwitchId switch_id, TableId table_id);

    // TODO: use RuleRange
    std::vector<RulePtr> rules();

    static bool isSpecialPort(PortId id);

private:
    std::map<SwitchId, SwitchPtr> switch_map_;

    // Special rules
    RulePtr drop_rule_;
    RulePtr controller_rule_;

};
