#include "Topology.hpp"

bool TopoId::operator<(const TopoId& other) const
{
   return switch_id < other.switch_id ?
          true : switch_id == other.switch_id ?
                 port_id < other.port_id : false;
}

bool TopoId::operator==(const TopoId& other) const
{
    return switch_id == other.switch_id && port_id == other.port_id;
}

Topology::Topology(Network& network):
        network_(network)
{
    
}

TopoId Topology::adjacentPort(SwitchId switch_id, PortId port_id) const
{
    TopoId topo_id{switch_id, port_id};
    auto it = port_map_.find(topo_id);
    return it != port_map_.end() ? it->second
                                 : TopoId{0, SpecialPort::NONE};
}

bool Topology::addLink(SwitchId src_switch_id, PortId src_port_id,
                       SwitchId dst_switch_id, PortId dst_port_id)
{
    TopoId src_id{src_switch_id, src_port_id};
    TopoId dst_id{dst_switch_id, dst_port_id};

    // Check for port loopback
    if (src_id == dst_id) {
        return false;
    }
    
    // Add ports to port map
    auto src_it = port_map_.find(src_id);
    auto dst_it = port_map_.find(dst_id);
    if (src_it == port_map_.end() && dst_it == port_map_.end()) {
        // All ports are available
        port_map_[src_id] = dst_id;
        port_map_[dst_id] = src_id;

        // DEBUG LOG
        std::cout<<"Link "<<src_switch_id<<":"<<src_port_id<<" <-> "
                          <<dst_switch_id<<":"<<dst_port_id<<std::endl;
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

void Topology::deleteLink(SwitchId src_switch_id, PortId src_port_id,
                          SwitchId dst_switch_id, PortId dst_port_id)
{
    TopoId src_id{src_switch_id, src_port_id};
    TopoId dst_id{dst_switch_id, dst_port_id};

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

void Topology::addRule(RulePtr new_rule)
{
    SwitchId switch_id = new_rule->switchId();
    SwitchPtr sw = network_.getSwitch(switch_id);
    auto sw_ports = sw->ports();
    
    // Compute out port dependency
    for (auto& action : new_rule->actions()) {
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
                    out_port_dependency_[topo_id].push_back(new_rule);
                }
                break;
            default: // Normal port
                // Check port existence
                if (std::find(sw_ports.begin(),
                              sw_ports.end(),
                              action.port_id) == sw_ports.end())
                { 
                    // TODO: Error, no such port
                }
                else {
                    TopoId topo_id{sw->id(), action.port_id};
                    out_port_dependency_[topo_id].push_back(new_rule);
                }
                break;
            }
            break;
        case ActionType::TABLE:
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
    if (new_rule->tableId() == sw->getFrontTable()->id()) {
        PortId in_port = new_rule->inPort();
        switch (in_port) {
        case SpecialPort::NONE:
            // Error, every rule must get packets from some port
            break;
        case SpecialPort::ANY:
            for (auto& port_id : sw_ports) {
                TopoId topo_id{sw->id(), port_id};
                // TODO: use sorted rule map
                in_port_dependency_[topo_id].push_back(new_rule);
            }
            break;
        default:
            if (std::find(sw_ports.begin(),
                          sw_ports.end(),
                          in_port) == sw_ports.end())
            { 
                // TODO: Error, no such port
            }
            else {
                TopoId topo_id{sw->id(), in_port};
                in_port_dependency_[topo_id].push_back(new_rule);
            }
            break;
        }
    }
}

std::vector<RulePtr> Topology::outRules(SwitchId switch_id,
                                        PortId port_id) const
{
    TopoId topo_id{switch_id, port_id};
    auto it = out_port_dependency_.find(topo_id);
    return it != out_port_dependency_.end()
           ? it->second
           : std::vector<RulePtr>();
}

std::vector<RulePtr> Topology::inRules(SwitchId switch_id,
                                       PortId port_id) const
{
    TopoId topo_id{switch_id, port_id};
    auto it = in_port_dependency_.find(topo_id);
    return it != in_port_dependency_.end()
           ? it->second
           : std::vector<RulePtr>();
}
