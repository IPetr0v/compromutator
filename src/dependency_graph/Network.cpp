#include "Network.hpp"

Rule::Rule(SwitchId switch_id, TableId table_id, RuleId id,
           uint16_t priority, Match& match,
           std::vector<Action>& action_list):
    switch_id_(switch_id), table_id_(table_id), id_(id),
    priority_(priority), match_(match), action_list_(action_list)
{
    // TODO: check action list for output ports
    //out_ports_.push_back();
}

Table::Table(SwitchId switch_id, TableId id):
    switch_id_(switch_id), id_(id)
{
    
}

RulePtr Table::getRule(RuleId id)
{
    auto it = rule_map_.find(id);
    return it != rule_map_.end() ?
                 it->second : nullptr;
}

RulePtr Table::addRule(RuleId rule_id, uint16_t priority, Match& match,
                       std::vector<Action>& action_list)
{
    RulePtr rule = std::make_shared<Rule>(switchId(), this->id(), rule_id,
                                          priority, match, action_list);
    
    // Check existing rule
    auto it = rule_map_.find(rule_id);
    return it == rule_map_.end() ?
                 rule_map_[rule_id] = rule : it->second;
}

void Table::deleteRule(RuleId id)
{
    rule_map_.erase(id);
}

Switch::Switch(SwitchId id, std::vector<PortId>& port_list):
    id_(id)
{
    // Add reserved ports
    addPort(ReservedPort::DROP);
    addPort(ReservedPort::IN_PORT);
    addPort(ReservedPort::ALL);
    addPort(ReservedPort::CONTROLLER);
    
    // Add normal ports
    for (auto& port_id : port_list) {
        addPort(port_id);
    }
    
    // Create front table
    TableId front_table_id = 0;
    addTable(front_table_id);
}

PortId Switch::addPort(PortId id)
{
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
    return it != table_map_.end() ?
                 it->second : nullptr;
}

TablePtr Switch::getFrontTable()
{
    TableId front_table_id = 0;
    return getTable(front_table_id);
}

TablePtr Switch::addTable(TableId table_id)
{
    TablePtr table = std::make_shared<Table>(this->id(), table_id);
    
    // Check existing table
    auto it = table_map_.find(table_id);
    return it == table_map_.end() ?
                 table_map_[table_id] = table : it->second;
}

void Switch::deleteTable(TableId id)
{
    table_map_.erase(id);
}

std::vector<PortId> Switch::ports()
{
    // Return all ports except reserved
    std::vector<PortId> port_list;
    for (auto& port_id : ports_) {
        if (!Network::isReservedPort(port_id))
            port_list.push_back(port_id);
    }
    
    return port_list;
}

Network::Network()
{
    
}

SwitchPtr Network::getSwitch(SwitchId id)
{
    auto it = switch_map_.find(id);
    return it != switch_map_.end() ?
                 it->second : nullptr;
}

SwitchPtr Network::addSwitch(SwitchId id, std::vector<PortId>& port_list)
{
    SwitchPtr sw = std::make_shared<Switch>(id, port_list);
    
    // Check existing switch
    auto it = switch_map_.find(sw->id());
    return it == switch_map_.end() ?
                 switch_map_[sw->id()] = sw : it->second;
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
                         Match& match, std::vector<Action>& action_list)
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

bool Network::isReservedPort(PortId id)
{
    switch (id) {
    case ReservedPort::DROP:
    case ReservedPort::IN_PORT:
    case ReservedPort::ALL:
    case ReservedPort::CONTROLLER:
        return true;
        
    default:
        return false;
    }
}
