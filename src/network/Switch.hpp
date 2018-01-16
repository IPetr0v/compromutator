#pragma once

#include "Rule.hpp"

#include <map>
#include <memory>
#include <vector>

class Table
{
public:
    Table(SwitchPtr sw, TableId id);
    ~Table();

    TableId id() const {return id_;}
    SwitchPtr sw() const {return sw_;}

    RulePtr rule(RuleId id) const;
    RulePtr addRule(Priority priority, NetworkSpace&& domain,
                    Actions&& actions);
    void deleteRule(RuleId id);

    RulePtr tableMissRule() const {return table_miss_rule_;}
    RuleRange rules() {return RuleRange(rule_map_);}
    RuleRange upperRules(RulePtr rule);
    RuleRange lowerRules(RulePtr rule);

private:
    TableId id_;
    SwitchPtr sw_;

    RuleMap rule_map_;
    RulePtr table_miss_rule_;

};

class Port
{
public:
    Port(SwitchPtr sw, PortId id);
    ~Port();

    PortId id() const {return id_;}
    TopoId topoId() const {return {switch_id_, id_};}
    SwitchPtr sw() const {return sw_;}

    RulePtr sourceRule() const {return source_rule_;}
    RulePtr sinkRule() const {return sink_rule_;}

    RulePtr addSrcRule(RulePtr rule) {return add_rule(rule, src_rules_);}
    RulePtr addDstRule(RulePtr rule) {return add_rule(rule, dst_rules_);}
    void deleteSrcRule(RulePtr rule) {delete_rule(rule, src_rules_);}
    void deleteDstRule(RulePtr rule) {delete_rule(rule, dst_rules_);}

    // Rules are sorted by a priority in a decreasing order
    RuleRange srcRules() {return RuleRange(src_rules_);}
    RuleRange dstRules() {return RuleRange(dst_rules_);}

private:
    PortId id_;
    SwitchPtr sw_;
    SwitchId switch_id_;

    RulePtr source_rule_;
    RulePtr sink_rule_;

    // Rules that send packets to the port connected to the getPort
    RuleMap src_rules_;
    // Rules that listen packets from the port connected to the getPort
    RuleMap dst_rules_;

    RulePtr add_rule(RulePtr rule, RuleMap& rule_map);
    void delete_rule(RulePtr rule, RuleMap& rule_map);
};

class Switch
{
    using PortRange = MapRange<std::map<PortId, PortPtr>>;
    using TableRange = MapRange<std::map<TableId, TablePtr>>;

public:
    Switch(SwitchId id, const std::vector<PortId>& ports,
           uint8_t table_number);
    ~Switch();

    SwitchId id() const {return id_;}
    bool isFrontTable(TablePtr table) const;

    PortPtr port(PortId id) const;
    TablePtr table(TableId id) const;
    PortRange ports() {return PortRange(port_map_);}
    TableRange tables() {return TableRange(table_map_);}

private:
    SwitchId id_;

    std::map<PortId, PortPtr> port_map_;
    std::map<TableId, TablePtr> table_map_;
    TablePtr front_table_;

    PortPtr add_port(PortId id);
    TablePtr add_table(TableId id);

};
