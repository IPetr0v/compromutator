#pragma once

#include "../Common.hpp"
#include "Network.hpp"

struct TopoId
{
    SwitchId switch_id;
    PortId port_id;
    bool operator<(const TopoId& other) const;
};

class Topology
{
public:
    Topology();
    
    bool addLink(SwitchPtr src_switch, PortId src_port_id,
                 SwitchPtr dst_switch, PortId dst_port_id);
    void deleteLink();
    
    //VertexRange inputTables(RulePtr rule);
    //VertexRange outputTables(RulePtr rule);
    
    const std::vector<RulePtr>& outRules(SwitchId switch_id, PortId port_id);
    const std::vector<RulePtr>& inRules(SwitchId switch_id, PortId port_id);

private:
    std::map<TopoId, TopoId> port_map_;
    std::map<TopoId, std::vector<RulePtr>> out_port_dependency_;
    std::map<TopoId, std::vector<RulePtr>> in_port_dependency_;

};
