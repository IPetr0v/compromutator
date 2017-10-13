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
    bool addLink(SwitchPtr src_switch, PortId src_port_id,
                 SwitchPtr dst_switch, PortId dst_port_id);
    void deleteLink(SwitchPtr src_switch, PortId src_port_id,
                    SwitchPtr dst_switch, PortId dst_port_id);

    void addRule(RulePtr new_rule);
    
    // TODO: may be not optimal, can we return references?
    std::vector<RulePtr> outRules(SwitchId switch_id, PortId port_id) const;
    std::vector<RulePtr> inRules(SwitchId switch_id, PortId port_id) const;

private:
    Network& network_;
    std::map<TopoId, TopoId> port_map_;
    std::map<TopoId, std::vector<RulePtr>> out_port_dependency_;
    std::map<TopoId, std::vector<RulePtr>> in_port_dependency_;

};
