#include "Network.hpp"

Table::Table(SwitchId switch_id, TableId id):
    switch_id_(switch_id), id_(id)
{
    
}

std::map<RuleId, RulePtr>* Table::get_rule_map(RuleId id)
{
    // Get priority
    auto priority_it = priority_map_.find(id);
    if (priority_it != priority_map_.end()) {
        Priority priority = priority_it->second;
        
        // Find RuleMap by priority
        auto sorted_rule_map_it = sorted_rule_map_.find(priority);
        return sorted_rule_map_it != sorted_rule_map_.end()
               ? &(sorted_rule_map_it->second)
               : &(priority_map_[priority]);
    }
    else
        return nullptr
}

RulePtr Table::get_rule_from_map(RuleId rule_id, std::map<RuleId, RulePtr>* rule_map)
{
    
}

RulePtr Table::getRule(RuleId id)
{
    auto it = rule_map_.find(id);
    return it != rule_map_.end() ? it->second : nullptr;
    
    // Experimental
    auto rule_map = get_rule_map(id);
    if (rule_map) {
        auto rule_it = rule_map.find(id);
        return rule_it != rule_map.end() ? rule_it->second
                                         : nullptr;
    }
    else
        return nullptr;
}

RulePtr Table::addRule(RuleId rule_id, uint16_t priority,
                       NetworkSpace& match, std::vector<Action>& action_list)
{
    RulePtr new_rule = std::make_shared<Rule>(switchId(), this->id(), rule_id,
                                              priority, match, action_list);
    
    // Check existing rule
    RulePtr old_rule = getRule(rule_id);
    return !old_rule ? rule_map_[rule_id] = new_rule
                     : old_rule;
    
    // Experimental
    // Check existing rule
    auto rule_map = get_rule_map(id);
    if (rule_map) {
        auto it = rule_map.find(rule_id);
        return it != rule_map.end()
               ? it->second // Old rule
               : rule_map[rule_id] = new_rule;
    }
    else {
        priority_map_[rule_id] = priority;
        
        RuleMap rule_map = {{rule_id, new_rule}};
        sorted_rule_map_[priority] = rule_map;
        
        return new_rule
    }
}

void Table::deleteRule(RuleId id)
{
    rule_map_.erase(id);
    
    // Experimental
    // Get priority
    auto priority_it = priority_map_.find(id);
    if (priority_it != priority_map_.end()) {
        Priority priority = priority_it->second;
        
        // Get rule map
        auto sorted_rule_map_it = sorted_rule_map_.find(priority);
        if (sorted_rule_map_it != rule_map_.end()) {
            RuleMap& rule_map = sorted_rule_map_it->second;
            
            // Delete rule
            rule_map.erase(id);
            if (rule_map.empty()) {
                sorted_rule_map_.erase(sorted_rule_map_it);
                priority_map_.erase(priority_it);
            }
        }
        else
            priority_map_.erase(priority_it);
    }
}

const RuleRange Table::upperRules(RulePtr rule)
{
    return RuleRange(sorted_rule_map_.upper_bound(new_rule->priority()));
}

const RuleRange Table::lowerRules(RulePtr rule)
{
    return RuleRange(sorted_rule_map_.lower_bound(new_rule->priority()));
}

Switch::Switch(SwitchId id, std::vector<PortId>& port_list):
    id_(id)
{
    for (auto& port_id : port_list)
        addPort(port_id);
    
    // Create front table
    TableId front_table_id = 0;
    addTable(front_table_id);
}

PortId Switch::addPort(PortId id)
{
    // Do not create special ports
    if (!isSpecialPort(id))
        ports_.push_back(id);
    return id;
}

void Switch::deletePort(PortId id)
{
    auto it = std::find(ports_.begin(), ports_.end(), id);
    if (it != ports_.end()) ports_.erase(it);
}

TablePtr Switch::getTable(TableId id)
{
    auto it = table_map_.find(id);
    return it != table_map_.end() ? it->second : nullptr;
}

TablePtr Switch::getFrontTable()
{
    TableId front_table_id = 0;
    return getTable(front_table_id);
}

TablePtr Switch::addTable(TableId table_id)
{
    TablePtr new_table = std::make_shared<Table>(this->id(), table_id);
    
    // Check existing table
    TablePtr old_table = getTable(table_id);
    return !old_table ? table_map_[table_id] = new_table
                      : old_table;
}

void Switch::deleteTable(TableId id)
{
    table_map_.erase(id);
}

std::vector<PortId> Switch::ports()
{
    // Return all ports except reserved
    std::vector<PortId> port_list;
    for (auto& port_id : ports_)
        port_list.push_back(port_id);
    
    return port_list;
}

Network::Network()
{
    
}

SwitchPtr Network::getSwitch(SwitchId id)
{
    auto it = switch_map_.find(id);
    return it != switch_map_.end() ? it->second : nullptr;
}

SwitchPtr Network::addSwitch(SwitchId id, std::vector<PortId>& port_list)
{
    SwitchPtr new_switch = std::make_shared<Switch>(id, port_list);
    
    // Check existing switch
    SwitchPtr old_switch = getSwitch(id);
    return !old_switch ? switch_map_[id] = new_switch
                       : old_switch;
}

void Network::deleteSwitch(SwitchId id)
{
    switch_map_.erase(id);
}

TablePtr Network::getTable(SwitchId switch_id, TableId table_id)
{
    SwitchPtr sw = getSwitch(switch_id);
    return sw ? sw->getTable(table_id) : nullptr;
}

TablePtr Network::addTable(SwitchId switch_id, TableId table_id)
{
    SwitchPtr sw = getSwitch(switch_id);
    return sw ? sw->addTable(table_id) : nullptr;
}

void Network::deleteTable(SwitchId switch_id, TableId table_id)
{
    SwitchPtr sw = getSwitch(switch_id);
    if (sw) sw->deleteTable(table_id);
}

RulePtr Network::getRule(SwitchId switch_id,
                         TableId table_id,
                         RuleId rule_id)
{
    SwitchPtr sw = getSwitch(switch_id);
    TablePtr table = sw ? sw->getTable(table_id) : nullptr;
    return table ? table->getRule(rule_id) : nullptr;
}

RulePtr Network::addRule(SwitchId switch_id, TableId table_id,
                         RuleId rule_id, uint16_t priority,
                         NetworkSpace& match,
                         std::vector<Action>& action_list)
{
    SwitchPtr sw = getSwitch(switch_id);
    TablePtr table = sw ? sw->getTable(table_id) : nullptr;
    return table ? table->addRule(rule_id, priority,
                                  match, action_list)
                 : nullptr;
}

void Network::deleteRule(SwitchId switch_id,
                         TableId table_id,
                         RuleId rule_id)
{
    SwitchPtr sw = getSwitch(switch_id);
    TablePtr table = sw ? sw->getTable(table_id) : nullptr;
    if (table) table->deleteRule(rule_id);
}

bool Network::isSpecialPort(PortId id)
{
    switch (id) {
    case SpecialPort::NONE:
    case SpecialPort::ANY:
        return true;
        
    default:
        return false;
    }
}
