#include "Topology.hpp"

bool TopoId::operator<(const TopoId& other) const
{
   return switch_id < other.switch_id ?
          true : switch_id == other.switch_id ?
                 port_id < other.port_id : false;
}

Topology::Topology()
{
    
}

const PortId Topology::adjacentPort(SwitchId switch_id, PortId port_id)
{
    TopoId topo_id{switch_id, port_id};
    auto it = port_map_.find(topo_id);
    return it != port_map_.end() ? it->second
                                 : TopoId{0, SpecialPort::NONE};
}

bool Topology::addLink(SwitchPtr src_switch, PortId src_port_id,
                       SwitchPtr dst_switch, PortId dst_port_id)
{
    TopoId src_id{src_switch->id(), src_port_id};
    TopoId dst_id{dst_switch->id(), dst_port_id};
    
    // Check for port loopback
    if (src_id == dst_id) {
        return false;
    }
    
    // Add ports to port map
    auto src_it = port_map_.find(src_id);
    auto dst_it = port_map_.find(dst_id);
    if (src_it == port_map_.end() && dst_it == port_map_.end()) {
        // All ports are avaivable
        port_map_[src_id] = dst_id;
        port_map_[dst_id] = src_id;
    }
    else {
        // TODO: think what to do with existing links
        // and partial links (one port is in topology
        // and the other is not)
        // Do not insert link, because we need to
        // recompute dependency
        return false;
    }
    
    return true;
}

void Topology::deleteLink()
{
    // Check if the link exists
    auto src_it = port_map_.find(src_id);
    auto dst_it = port_map_.find(dst_id);
    if (src_it != port_map_.end() &&
        dst_it != port_map_.end())
    {
        port_map_.erase(src_it);
        port_map_.erase(dst_it);
    }
}

Topology::newRule(RulePtr rule)
{
    SwitchId switch_id = rule->switch_id();
    SwitchPtr sw = network_->getSwitch(switch_id);
    std::vector<PortId>& sw_ports = sw->ports();
    
    // Compute out port dependency
    for (auto& action : rule->actions()) {
        switch (action.type) {
        case ActionType::PORT:
            switch (action.port_id) {
            case PortAction::DROP:
            case PortAction::CONTROLLER:
                // Not a normal port
                break;
            case PortAction::IN_PORT:
            case PortAction::ALL:
                // In this case every port may be the outport
                for (auto& port_id : sw_ports) {
                    TopoId topo_id{sw->id(), port_id};
                    // TODO: use sorted rule map
                    out_port_dependency_[topo_id].second.push_back(rule);
                }
                break;
            default: // Normal port
                // Check port existence
                if (std::find(sw_ports.begin(),
                              sw_ports.end(),
                              port_id) == sw_ports.end())
                { 
                    // TODO: Error, no such port
                }
                else {
                    TopoId topo_id{sw->id(), port_id};
                    out_port_dependency_[topo_id].second.push_back(rule);
                }
                break;
            }
            break;
        case ActionType::TABLE:
            addDependencies(rule, action.table_id, transfer);
            break;
        case ActionType::GROUP:
            break;
        default:
            // TODO: Error, undefined action,
            // or may be enum class prevents this
            break;
        }
    }
    
    // Compute in port dependency
    if (rule->tableId() == sw->frontTable()->id()) {
        PortId in_port = rule->inPort();
        switch (in_port) {
        case SpecialPort::NONE:
            // Error, every rule must get packets from some port
            break;
        case SpecialPort::ANY:
            for (auto& port_id : sw_ports) {
                TopoId topo_id{sw->id(), port_id};
                // TODO: use sorted rule map
                in_port_dependency_[topo_id].second.push_back(rule);
            }
            break;
        default:
            if (std::find(sw_ports.begin(),
                          sw_ports.end(),
                          port_id) == sw_ports.end())
            { 
                // TODO: Error, no such port
            }
            else {
                TopoId topo_id{sw->id(), port_id};
                in_port_dependency_[topo_id].second.push_back(rule);
            }
            break;
        }
    }
}

const std::vector<RulePtr> Topology::outRules(SwitchId switch_id,
                                              PortId port_id)
{
    TopoId topo_id{switch_id, port_id};
    auto it = out_port_dependency_.find(topo_id);
    return it != out_port_dependency_.end()
           ? it->second
           : std::vector<RulePtr>();
}

const std::vector<RulePtr>& Topology::inRules(SwitchId switch_id,
                                              PortId port_id)
{
    TopoId topo_id{switch_id, port_id};
    auto it = in_port_dependency_.find(topo_id);
    return it != in_port_dependency_.end()
           ? it->second
           : std::vector<RulePtr>();
}
