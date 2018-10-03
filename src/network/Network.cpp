#include "Network.hpp"

Network::Network()
{
    // Create special rules
    drop_rule_ = new Rule(RuleType::SINK, nullptr, nullptr,
                          LOW_PRIORITY, ZERO_COOKIE,
                          NetworkSpace::wholeSpace(),
                          Actions::noActions());
    controller_rule_ = new Rule(RuleType::SINK, nullptr, nullptr,
                                LOW_PRIORITY, ZERO_COOKIE,
                                NetworkSpace::wholeSpace(),
                                Actions::noActions());
}

Network::~Network()
{
    // Delete switches
    for (auto it : switch_map_) {
        delete it.second;
    }
    switch_map_.clear();

    delete drop_rule_;
    delete controller_rule_;
}

SwitchPtr Network::getSwitch(SwitchId id) const
{
    auto it = switch_map_.find(id);
    return it != switch_map_.end() ? it->second : nullptr;
}

SwitchPtr Network::addSwitch(const SwitchInfo& info)
{
    auto existing_sw = getSwitch(info.id);
    if (not existing_sw) {
        auto sw = switch_map_[info.id] = new Switch(info);
        for (auto table : sw->tables()) {
            add_rule_to_topology(table->tableMissRule());
        }
        return sw;
    }
    else {
        return existing_sw;
    }
}

void Network::deleteSwitch(SwitchId id)
{
    auto sw = getSwitch(id);
    if (not sw) return;

    // Delete links
    for (auto src_port : sw->ports()) {
        auto dst_port = adjacentPort(src_port);
        if (dst_port) {
            deleteLink(src_port->topoId(), dst_port->topoId());
        }
    }

    auto it = switch_map_.find(id);
    if (it != switch_map_.end()) {
        delete it->second;
        switch_map_.erase(it);
    }
}

TablePtr Network::getTable(SwitchId switch_id, TableId table_id) const
{
    SwitchPtr sw = getSwitch(switch_id);
    return sw ? sw->table(table_id) : nullptr;
}

PortPtr Network::getPort(TopoId topo_id) const
{
    auto switch_id = topo_id.first;
    auto port_id = topo_id.second;
    SwitchPtr sw = getSwitch(switch_id);
    return sw ? sw->port(port_id) : nullptr;
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

RulePtr Network::rule(RuleId id) const
{
    auto switch_id = std::get<0>(id);
    auto table_id = std::get<1>(id);

    SwitchPtr sw = getSwitch(switch_id);
    TablePtr table = sw ? sw->table(table_id) : nullptr;
    return table ? table->rule(id) : nullptr;
}

RulePtr Network::rule(SwitchId switch_id, TableId table_id,
                      Priority priority, const Match& match)
{
    SwitchPtr sw = getSwitch(switch_id);
    TablePtr table = sw ? sw->table(table_id) : nullptr;
    if (table) {
        return table->rule(priority, match);
    }
    else {
        return nullptr;
    }
}

RulePtr Network::addRule(SwitchId switch_id, TableId table_id,
                         Priority priority, Cookie cookie,
                         Match&& match, ActionsBase&& actions_base)
{
    // Get Table
    SwitchPtr sw = getSwitch(switch_id);
    TablePtr table = sw ? sw->table(table_id) : nullptr;
    if (not table) {
        auto msg = "Wrong table:" + std::to_string(table_id) +
                   " for dpid:" + std::to_string(switch_id);
        throw std::invalid_argument(msg);
    }

    // Create rule
    // TODO: check group existence
    auto actions_pair = get_actions(sw, std::move(actions_base));
    bool success = actions_pair.first;
    if (not success) {
        // TODO: put this rule on hold
        std::cerr << "Network error: Wrong rule actions" << std::endl;
        return nullptr;
    }
    auto actions = std::move(actions_pair.second);
    RulePtr rule = table->addRule(priority, cookie, std::move(match),
                                  std::move(actions));
    if (not rule) return nullptr;

    //std::cout<<"[Network] ADD "<<rule<<std::endl;

    // Update topology
    add_rule_to_topology(rule);
    return rule;
}

void Network::deleteRule(RuleId id)
{
    auto switch_id = std::get<0>(id);
    auto table_id = std::get<1>(id);

    SwitchPtr sw = getSwitch(switch_id);
    TablePtr table = sw ? sw->table(table_id) : nullptr;
    RulePtr rule = table ? table->rule(id) : nullptr;
    if (rule && table) {
        //std::cout<<"[Network] DELETE "<<rule<<std::endl;

        delete_rule_from_topology(rule);
        table->deleteRule(id);
    }
}

PortPtr Network::adjacentPort(PortPtr port) const
{
    auto it = topology_.find(port->topoId());
    return it != topology_.end() ? it->second : nullptr;
}

std::pair<Link, bool> Network::link(TopoId src_topo_id, TopoId dst_topo_id)
{
    // Check getPort existence
    auto src_port = getPort(src_topo_id);
    auto dst_port = getPort(dst_topo_id);
    if (not src_port || not dst_port) return std::make_pair(Link(), false);

    // Get the link
    auto src_it = topology_.find(src_topo_id);
    auto dst_it = topology_.find(dst_topo_id);
    if (src_it != topology_.end() && dst_it != topology_.end() &&
        src_it->second->topoId() == dst_port->topoId() &&
        dst_it->second->topoId() == src_port->topoId()) {
        return std::make_pair(Link{src_port, dst_port}, true);
    }
    return std::make_pair(Link(), false);
}

std::pair<Link, bool> Network::addLink(TopoId src_topo_id, TopoId dst_topo_id)
{
    // Check for loopbacks
    if (src_topo_id == dst_topo_id) return std::make_pair(Link(), false);

    // Check getPort existence
    auto src_port = getPort(src_topo_id);
    auto dst_port = getPort(dst_topo_id);
    if (not src_port || not dst_port) return std::make_pair(Link(), false);

    // Add link to the topology
    auto src_it = topology_.find(src_topo_id);
    auto dst_it = topology_.find(dst_topo_id);
    if (src_it == topology_.end() && dst_it == topology_.end()) {
        // All ports are available
        topology_.emplace(src_topo_id, dst_port);
        topology_.emplace(dst_topo_id, src_port);
    }
    else {
        return std::make_pair(Link(), false);
    }

    return std::make_pair(Link{src_port, dst_port}, true);
}

std::pair<Link, bool> Network::deleteLink(TopoId src_topo_id,
                                          TopoId dst_topo_id)
{
    // Check for loopbacks
    if (src_topo_id == dst_topo_id) std::make_pair(Link(), false);

    // Check getPort existence
    auto src_port = getPort(src_topo_id);
    auto dst_port = getPort(dst_topo_id);
    if (not src_port || not dst_port) return std::make_pair(Link(), false);

    // Delete link from the topology
    auto src_it = topology_.find(src_topo_id);
    auto dst_it = topology_.find(dst_topo_id);
    if (src_it != topology_.end() && dst_it != topology_.end()) {
        topology_.erase(src_it);
        topology_.erase(dst_it);
        return std::make_pair(Link{src_port, dst_port}, true);
    }
    return std::make_pair(Link(), false);
}

std::pair<bool, Actions> Network::get_actions(SwitchPtr sw,
                                              ActionsBase&& actions_base) const
{
    Actions actions;
    for (auto& port_action_base : actions_base.port_actions) {
        auto port_type = port_action_base.port_type;
        if (port_type == PortType::NORMAL) {
            auto port_id = port_action_base.port_id;
            auto port = getPort({sw->id(), port_id});
            if (port) {
                PortAction port_action(std::move(port_action_base), port);
                actions.port_actions.emplace_back(std::move(port_action));
            }
            else {
                return std::make_pair(false, std::move(actions));
            }
        }
        else {
            PortAction port_action(std::move(port_action_base), nullptr);
            actions.port_actions.emplace_back(std::move(port_action));
        }
    }
    for (auto& table_action_base : actions_base.table_actions) {
        auto table_id = table_action_base.table_id;
        auto table = getTable(sw->id(), table_id);
        if (table) {
            TableAction table_action(std::move(table_action_base), table);
            actions.table_actions.emplace_back(std::move(table_action));
        }
        else {
            return std::make_pair(false, std::move(actions));
        }
    }
    // TODO: implement group action parsing
    // for (auto& group_action_base : actions_base.group_actions) {
    //
    // }
    return std::make_pair(true, std::move(actions));
}

void Network::add_rule_to_topology(RulePtr rule)
{
    auto sw = rule->sw();
    auto table = rule->table();

    // Update src rules for ports
    for (const auto& port_action : rule->actions().port_actions) {
        switch (port_action.port_type) {
        case PortType::NORMAL:
            port_action.port->addSrcRule(rule);
            break;
        case PortType::ALL:
        case PortType::IN_PORT:
            for (auto port : sw->ports()) {
                port->addSrcRule(rule);
            }
            break;
        default:
            break;
        }
    }

    // Update dst rules for ports
    if (table->isFrontTable()) {
        auto in_port = sw->port(rule->inPort());
        if (in_port) {
            // Rule listens to a one specified getPort
            in_port->addDstRule(rule);
        }
        else {
            // Rule listens to all ports
            for (auto port : sw->ports()) {
                port->addDstRule(rule);
            }
        }
    }
}

void Network::delete_rule_from_topology(RulePtr rule)
{
    auto sw = rule->sw();
    auto table = rule->table();

    // Update src rules for ports
    for (const auto& port_action : rule->actions().port_actions) {
        switch (port_action.port_type) {
        case PortType::NORMAL:
            port_action.port->deleteSrcRule(rule);
            break;
        case PortType::ALL:
        case PortType::IN_PORT:
            for (auto port : sw->ports()) {
                port->deleteSrcRule(rule);
            }
            break;
        default:
            break;
        }
    }

    // Update dst rules for ports
    if (table->isFrontTable()) {
        auto in_port = sw->port(rule->inPort());
        if (in_port) {
            // Rule listens to a one specified getPort
            in_port->deleteDstRule(rule);
        }
        else {
            // Rule listens to all ports
            for (auto port : sw->ports()) {
                port->deleteDstRule(rule);
            }
        }
    }
}
