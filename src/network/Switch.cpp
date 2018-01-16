#include "Switch.hpp"

#include <algorithm>

Table::Table(SwitchPtr sw, TableId id):
    id_(id), sw_(sw)
{
    // Create a getTable miss rule
    table_miss_rule_ = addRule(LOW_PRIORITY, NetworkSpace::wholeSpace(),
                               Actions::dropAction());
}

Table::~Table()
{
    // Delete rules
    for (auto it : rule_map_) {
        delete it.second;
    }
    rule_map_.clear();
}

RulePtr Table::rule(RuleId id) const
{
    auto it = rule_map_.find(id);
    return it != rule_map_.end() ? it->second : nullptr;
}

RulePtr Table::addRule(Priority priority, NetworkSpace&& domain,
                       Actions&& actions)
{
    auto rule = new Rule(RuleType::FLOW, sw_, this, priority,
                         std::move(domain), std::move(actions));
    return rule_map_[rule->id()] = rule;
}

void Table::deleteRule(RuleId id)
{
    auto it = rule_map_.find(id);
    if (it != rule_map_.end()) {
        delete it->second;
        rule_map_.erase(it);
    }
}

RuleRange Table::upperRules(RulePtr rule)
{
    auto it = rule_map_.lower_bound(rule->id());
    auto lower_bound = std::find_if(it, rule_map_.end(),
        [rule](const std::pair<RuleId, RulePtr>& rule_pair) -> bool {
            auto lower_rule = rule_pair.second;
            return lower_rule->priority() < rule->priority();
        }
    );
    return {rule_map_, rule_map_.begin(), lower_bound};
}

RuleRange Table::lowerRules(RulePtr rule)
{
    auto it = rule_map_.upper_bound(rule->id());
    auto upper_bound = std::find_if(it, rule_map_.end(),
        [rule](const std::pair<RuleId, RulePtr>& rule_pair) -> bool {
            auto upper_rule = rule_pair.second;
            return upper_rule->priority() > rule->priority();
        }
    );
    // TODO: CRITICAL - check std::greater<> with upper/lower bounds
    return {rule_map_, upper_bound, rule_map_.end()};
}

Port::Port(SwitchPtr sw, PortId id):
    id_(id), sw_(sw), switch_id_(sw->id())
{
    source_rule_ = new Rule(RuleType::SOURCE, sw_, nullptr, LOW_PRIORITY,
                            NetworkSpace::wholeSpace(), Actions::noActions());
    sink_rule_ = new Rule(RuleType::SINK, sw_, nullptr, LOW_PRIORITY,
                          NetworkSpace::wholeSpace(), Actions::noActions());
}

Port::~Port()
{
    delete source_rule_;
    delete sink_rule_;
}

RulePtr Port::add_rule(RulePtr rule, RuleMap& rule_map)
{
    return rule_map[rule->id()] = rule;
}

void Port::delete_rule(RulePtr rule, RuleMap& rule_map)
{
    rule_map.erase(rule->id());
}

Switch::Switch(SwitchId id, const std::vector<PortId>& ports,
               uint8_t table_number):
    id_(id)
{
    // Create ports
    for (auto port_id : ports) {
        add_port(port_id);
    }

    // Create tables
    front_table_ = add_table(0);
    for (TableId table_id = 1; table_id < table_number; table_id++) {
        add_table(table_id);
    }
}

Switch::~Switch()
{
    // Delete ports
    for (auto it : port_map_) {
        delete it.second;
    }
    port_map_.clear();

    // Delete tables
    for (auto it : table_map_) {
        delete it.second;
    }
    table_map_.clear();
}

bool Switch::isFrontTable(TablePtr table) const
{
    return (table->sw()->id() == id_) && (table->id() == front_table_->id());
}

PortPtr Switch::port(PortId id) const
{
    auto it = port_map_.find(id);
    return it != port_map_.end() ? it->second : nullptr;
}

PortPtr Switch::add_port(PortId id)
{
    // Check existing getPort
    auto old_port = port(id);
    return old_port ? old_port : port_map_[id] = new Port(this, id);
}

TablePtr Switch::table(TableId id) const
{
    auto it = table_map_.find(id);
    return it != table_map_.end() ? it->second : nullptr;
}

TablePtr Switch::add_table(TableId id)
{
    // Check existing getTable
    TablePtr old_table = table(id);
    return old_table ? old_table : table_map_[id] = new Table(this, id);
}
