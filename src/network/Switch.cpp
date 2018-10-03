#include "Switch.hpp"

#include <algorithm>

Table::Table(SwitchPtr sw, TableId id):
    id_(id), sw_(sw)
{
    // Create a getTable miss rule
    table_miss_rule_ = addRule(ZERO_PRIORITY, ZERO_COOKIE,
                               NetworkSpace::wholeSpace().match(),
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

RulePtr Table::rule(Priority priority, const Match& match)
{
    RuleId id(sw_->id(), id_, priority, 0x0);
    //auto upper_bound = rule_map_.upper_bound(id);
    auto it = std::find_if(rule_map_.begin(), rule_map_.end(),//upper_bound,
        [priority, match](const std::pair<RuleId, RulePtr>& rule_pair) -> bool {
            auto lower_rule = rule_pair.second;
            if (priority == lower_rule->priority()) {
                return match == lower_rule->match();
            }
            else {
                return false;
            }
        }
    );
    return it != rule_map_.end() ? it->second : nullptr;
}

RulePtr Table::addRule(Priority priority, Cookie cookie,
                       Match&& match, Actions&& actions)
{
    // TODO: Check rule rewrite (and table-miss rewrite)
    auto rule = new Rule(RuleType::FLOW, sw_, this, priority, cookie,
                         std::move(match), std::move(actions));
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
    auto it = rule_map_.upper_bound(rule->id());
    RuleMap::reverse_iterator reverse_it(it);
    auto upper_bound = std::find_if(reverse_it, rule_map_.rend(),
        [rule](const std::pair<RuleId, RulePtr>& rule_pair) -> bool {
            auto upper_rule = rule_pair.second;
            return rule->priority() < upper_rule->priority();
        }
    );
    return {rule_map_, rule_map_.begin(), upper_bound.base()};
}

RuleRange Table::lowerRules(RulePtr rule)
{
    auto it = rule_map_.lower_bound(rule->id());
    auto lower_bound = std::find_if(it, rule_map_.end(),
        [rule](const std::pair<RuleId, RulePtr>& rule_pair) -> bool {
            auto lower_rule = rule_pair.second;
            return rule->priority() > lower_rule->priority();
        }
    );
    // TODO: CRITICAL - check std::greater<> with upper/lower bounds
    return {rule_map_, lower_bound, rule_map_.end()};
}

bool Table::isFrontTable() const
{
    return sw_->frontTable()->id() == id_;
}

Port::Port(SwitchPtr sw, PortInfo info):
    id_(info.id), speed_(info.speed), sw_(sw), switch_id_(sw->id())
{
    source_rule_ = new Rule(RuleType::SOURCE, sw_, nullptr,
                            LOW_PRIORITY, ZERO_COOKIE,
                            NetworkSpace(id_),
                            Actions::noActions());
                            //Actions::forwardAction(
                            //    sw_->frontTable()->id(), sw_->frontTable()));
    sink_rule_ = new Rule(RuleType::SINK, sw_, nullptr,
                          LOW_PRIORITY, ZERO_COOKIE,
                          NetworkSpace(id_), Actions::noActions());
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

Switch::Switch(const SwitchInfo& info):
    id_(info.id), table_number_(info.table_number)
{
    // Create tables
    front_table_ = addTable(0);

    // Create ports
    for (const auto& port : info.ports) {
        if (port.id != SpecialPort::LOCAL) {
            add_port(port);
        }
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

PortPtr Switch::port(PortId id) const
{
    auto it = port_map_.find(id);
    return it != port_map_.end() ? it->second : nullptr;
}

PortPtr Switch::add_port(PortInfo info)
{
    // Check existing getPort
    auto old_port = port(info.id);
    return old_port ? old_port : port_map_[info.id] = new Port(this, info);
}

TablePtr Switch::table(TableId id) const
{
    auto it = table_map_.find(id);
    return it != table_map_.end() ? it->second : nullptr;
}

TablePtr Switch::addTable(TableId id)
{
    if (id < table_number_) {
        // Check existing getTable
        TablePtr old_table = table(id);
        if (not old_table) {
            return table_map_[id] = new Table(this, id);
        }
        return old_table;
    }
    else {
        return nullptr;
    }
}
