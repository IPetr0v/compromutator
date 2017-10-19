#pragma once

#include "../Common.hpp"
#include "Network.hpp"

#include <map>
#include <vector>

struct TopoId
{
    SwitchId switch_id;
    PortId port_id;
    bool operator<(const TopoId& other) const;
    bool operator==(const TopoId& other) const;
};

class Topology
{
public:
    explicit Topology(Network& network);

    TopoId adjacentPort(SwitchId switch_id, PortId port_id) const;
    bool addLink(SwitchId src_switch_id, PortId src_port_id,
                 SwitchId dst_switch_id, PortId dst_port_id);
    void deleteLink(SwitchId src_switch_id, PortId src_port_id,
                    SwitchId dst_switch_id, PortId dst_port_id);

    void addRule(RulePtr new_rule);
    
    // TODO: may be not optimal, can we return references?
    // Rules that send packets to the port connected to the port_id
    std::vector<RulePtr> srcRules(SwitchId switch_id, PortId port_id) const;
    // Rules that listen packets from the port connected to the port_id
    std::vector<RulePtr> dstRules(SwitchId switch_id, PortId port_id) const;

private:
    Network& network_;
    std::map<TopoId, TopoId> port_map_;
    std::map<TopoId, std::vector<RulePtr>> out_port_dependency_;
    std::map<TopoId, std::vector<RulePtr>> in_port_dependency_;

};
