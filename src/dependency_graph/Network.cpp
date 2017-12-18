#include "Network.hpp"

Table::Table(SwitchId switch_id, TableId id):
    switch_id_(switch_id), id_(id)
{
    // Create table miss rules
    NetworkSpace domain; // Whole network space
    Action action;
    action.type = ActionType::PORT;
    action.port_id = PortAction::DROP;
    std::vector<Action> actions = {action};

    table_miss_rule_ = addRule(0, domain, actions);
}

RulePtr Table::getRule(RuleId id)
{
    // Get priority
    auto priority_it = priority_map_.find(id);
    if (priority_it != priority_map_.end()) {
        Priority priority = priority_it->second;

        // Find RuleMap by priority
        auto sorted_rule_map_it = sorted_rule_map_.find(priority);
        if (sorted_rule_map_it != sorted_rule_map_.end()) {
            RuleMap& rule_map = sorted_rule_map_it->second;

            // Get rule
            auto rule_it = rule_map.find(id);
            return rule_it != rule_map.end() ? rule_it->second : nullptr;
        }
        else
            return nullptr;
    }
    else
        return nullptr;
}

RulePtr Table::addRule(uint16_t priority, NetworkSpace& domain,
                       std::vector<Action>& action_list)
{
    RulePtr new_rule = std::make_shared<Rule>(RuleType::FLOW, switchId(),
                                              this->id(), priority,
                                              domain, action_list);
    auto rule_id = new_rule->id();

    // Get priority
    auto priority_it = priority_map_.find(rule_id);
    if (priority_it != priority_map_.end()) {

        // Find RuleMap by priority
        auto sorted_rule_map_it = sorted_rule_map_.find(priority);
        if (sorted_rule_map_it != sorted_rule_map_.end()) {
            RuleMap& rule_map = sorted_rule_map_it->second;

            // Get rule
            auto rule_it = rule_map.find(rule_id);
            // TODO: check map[] return type (if its a reference or RulePtr)
            return rule_it != rule_map.end() ? rule_it->second // Old rule
                                             : rule_map[rule_id] = new_rule;
        }
        else {
            RuleMap new_rule_map = {{rule_id, new_rule}};
            sorted_rule_map_[priority] = new_rule_map;

            return new_rule;
        }
    }
    else {
        priority_map_[rule_id] = priority;

        RuleMap new_rule_map = {{rule_id, new_rule}};
        sorted_rule_map_[priority] = new_rule_map;

        return new_rule;
    }
}

void Table::deleteRule(RuleId id)
{
    // Get priority
    auto priority_it = priority_map_.find(id);
    if (priority_it != priority_map_.end()) {
        Priority priority = priority_it->second;
        
        // Get rule map
        auto sorted_rule_map_it = sorted_rule_map_.find(priority);
        if (sorted_rule_map_it != sorted_rule_map_.end()) {
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

RuleRange Table::upperRules(RulePtr rule)
{
    auto lower_bound = sorted_rule_map_.lower_bound(rule->priority());
    return {sorted_rule_map_, sorted_rule_map_.begin(), lower_bound};
}

RuleRange Table::lowerRules(RulePtr rule)
{
    std::cout<<"lowerRules sorted_rule_map_.size="<<sorted_rule_map_.size()
             <<std::endl;
    auto upper_bound = sorted_rule_map_.upper_bound(rule->priority());
    return {sorted_rule_map_, upper_bound, sorted_rule_map_.end()};
}

Port::Port(SwitchId switch_id, PortId id):
    switch_id_(switch_id), id_(id)
{
    NetworkSpace domain; // Whole network space
    std::vector<Action> actions = {};
    source_rule_ = std::make_shared<Rule>(RuleType::SOURCE, switch_id_,
                                          0, 0, domain, actions);
    sink_rule_ = std::make_shared<Rule>(RuleType::SINK, switch_id_,
                                        0, 0, domain, actions);
}

Switch::Switch(SwitchId id, std::vector<PortId>& port_list):
    id_(id)
{
    // DEBUG LOG
    std::cout<<"Switch "<<id_<<std::endl;

    for (auto& port_id : port_list)
        addPort(port_id);
    
    // Create front table
    front_table_ = addTable(0);
}

PortPtr Switch::getPort(PortId id)
{
    auto it = port_map_.find(id);
    return it != port_map_.end() ? it->second : nullptr;
}

PortPtr Switch::addPort(PortId port_id)
{
    // Do not create special ports
    if (Network::isSpecialPort(port_id))
        return nullptr;

    PortPtr new_port = std::make_shared<Port>(this->id(), port_id);

    // Check existing table
    PortPtr old_port = getPort(port_id);
    // TODO: remove port vector and use only map
    //return !old_port ? port_map_[port_id] = new_port
    //                 : old_port;
    if (!old_port) {
        ports_.push_back(port_id);
        return port_map_[port_id] = new_port;
    }
    else {
        return old_port;
    }
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

std::vector<PortId>& Switch::portIdList()
{
    return ports_;
}

std::vector<PortPtr> Switch::ports()
{
    std::vector<PortPtr> port_list;
    for (const auto& port_pair : port_map_) {
        port_list.push_back(port_pair.second);
    }
    return port_list;
}

std::vector<TablePtr> Switch::tables()
{
    std::vector<TablePtr> table_list;
    for (const auto& table_pair : table_map_) {
        table_list.push_back(table_pair.second);
    }
    return table_list;
}

Network::Network()
{
    // Create special rules
    NetworkSpace domain; // Whole network space
    std::vector<Action> actions = {};
    drop_rule_ = std::make_shared<Rule>(RuleType::SINK, 0, 0, 0,
                                        domain, actions);
    controller_rule_ = std::make_shared<Rule>(RuleType::SINK, 0, 0, 0,
                                              domain, actions);
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

std::vector<RulePtr> Network::rules()
{
    std::vector<RulePtr> rule_list;
    rule_list.push_back(drop_rule_);
    rule_list.push_back(controller_rule_);
    for (const auto& switch_pair : switch_map_) {
        auto& sw = switch_pair.second;
        for (const auto& table : sw->tables()) {
            for (const auto& rule : table->rules()) {
                rule_list.push_back(rule);
            }
        }
        for (const auto& port : sw->ports()) {
            rule_list.push_back(port->sourceRule());
            rule_list.push_back(port->sinkRule());
        }
    }
    return rule_list;
}

RulePtr Network::getRule(RuleId rule_id)
{
    // TODO: CRITICAL rewrite without brute-force
    auto controller = this->controller_rule_;
    if (rule_id == controller->id()) return controller;
    auto drop = this->drop_rule_;
    if (rule_id == drop->id()) return drop;

    for (const auto sw_it : switch_map_) {
        const auto sw = sw_it.second;
        for (const auto table : sw->tables()) {
            auto rule = table->getRule(rule_id);
            if (nullptr != rule) return rule;
        }
        for (const auto port : sw->ports()) {
            auto source = port->sourceRule();
            if (rule_id == source->id()) return source;
            auto sink = port->sinkRule();
            if (rule_id == sink->id()) return sink;
        }
    }
    return nullptr;
}

RulePtr Network::getRule(SwitchId switch_id, TableId table_id, RuleId rule_id)
{
    SwitchPtr sw = getSwitch(switch_id);
    TablePtr table = sw ? sw->getTable(table_id) : nullptr;
    return table ? table->getRule(rule_id) : nullptr;
}

RulePtr Network::addRule(SwitchId switch_id, TableId table_id,
                         uint16_t priority, NetworkSpace& domain,
                         std::vector<Action>& action_list)
{
    // Create rule
    SwitchPtr sw = getSwitch(switch_id);
    TablePtr table = sw ? sw->getTable(table_id) : nullptr;
    return table ? table->addRule(priority, domain, action_list) : nullptr;
}

void Network::deleteRule(SwitchId switch_id, TableId table_id, RuleId rule_id)
{
    SwitchPtr sw = getSwitch(switch_id);
    TablePtr table = sw ? sw->getTable(table_id) : nullptr;
    if (table) table->deleteRule(rule_id);
}

RulePtr Network::sourceRule(SwitchId switch_id, PortId port_id)
{
    SwitchPtr sw = getSwitch(switch_id);
    PortPtr port = sw ? sw->getPort(port_id) : nullptr;
    return port ? port->sourceRule() : nullptr;
}

RulePtr Network::sinkRule(SwitchId switch_id, PortId port_id)
{
    SwitchPtr sw = getSwitch(switch_id);
    PortPtr port = sw ? sw->getPort(port_id) : nullptr;
    return port ? port->sinkRule() : nullptr;
}

RulePtr Network::tableMissRule(SwitchId switch_id, TableId table_id)
{
    SwitchPtr sw = getSwitch(switch_id);
    TablePtr table = sw ? sw->getTable(table_id) : nullptr;
    return table ? table->tableMissRule() : nullptr;
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
