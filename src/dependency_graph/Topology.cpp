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

bool Topology::addLink(SwitchPtr src_switch, PortId src_port_id,
                       SwitchPtr dst_switch, PortId dst_port_id)
{
    /*TopoId src_id{src_switch->id(), src_port_id};
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
    */
    return true;
}

void Topology::deleteLink()
{
    /*// Check if the link exists
    auto src_it = port_map_.find(src_id);
    auto dst_it = port_map_.find(dst_id);
    if (src_it != port_map_.end() &&
        dst_it != port_map_.end())
    {
        port_map_.erase(src_it);
        port_map_.erase(dst_it);
    }*/
}

/*Topology::newRule(SwitchPtr sw, RulePtr rule)
{
    // Compute out port dependency
    for (auto& port_id : rule->outPorts()) {
        // TODO: Check special cases (IN_PORT, ALL_PORT)
        
        TopoId topo_id{sw->id(), port_id};
        out_port_dependency_[topo_id].second.push_back(rule);
    }
    
    // Compute in port dependency
    if (rule->tableId() == sw->frontTable()->id()) {
        auto& in_ports = rule->inPorts();
        if (in_ports.empty()) {
            for (auto& port_id : sw->ports()) {
                TopoId topo_id{sw->id(), port_id};
                in_port_dependency_[topo_id].push_back(rule);
            }
        }
        else {
            for (auto& port_id : in_ports) {
                TopoId topo_id{sw->id(), port_id};
                in_port_dependency_[topo_id].push_back(rule);
            }
        }
    }
}*/

const std::vector<RulePtr>& Topology::outRules(SwitchId switch_id,
                                               PortId port_id)
{
    TopoId topo_id{switch_id, port_id};
    return out_port_dependency_[topo_id];
}

const std::vector<RulePtr>& Topology::inRules(SwitchId switch_id,
                                              PortId port_id)
{
    TopoId topo_id{switch_id, port_id};
    return in_port_dependency_[topo_id];
}

/*VertexRange Topology::inputTables(RulePtr rule)
{
    VertexRange input_vertex_range;
    return input_table_range;
}

VertexRange Topology::outputTables(RulePtr rule)
{
    VertexRange output_vertex_range;
    
    for (auto& state : rule->range()) {
        switch (state.location) {
        case PORT:
        case TABLE:
        case CONTROLLER:
        case DROPPED:
        default:
        }
        // To controller
        // To table
        // To port
        // Drop
    }
    
    return output_vertex_range;
}*/
