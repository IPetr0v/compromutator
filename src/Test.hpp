#pragma once

#include "Common.hpp"
#include "NetworkSpace.hpp"
#include "./detector/Detector.hpp"

#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include <cstring>
#include <vector>

using SwitchList = std::vector<SwitchId>;
using PortList = std::vector<PortId>;
using ActionList = std::vector<Action>;

struct Link
{
    SwitchId src_switch_id;
    PortId src_port_id;
    SwitchId dst_switch_id;
    PortId dst_port_id;
};
using LinkList = std::vector<Link>;

class Test
{

};

class DependencyGraphTest : public Test
{
public:
    DependencyGraphTest() = default;

    // Tests
    bool simpleDependencyTest();

private:
    void create_switches(SwitchList switches, PortList ports);
    void create_topology(LinkList links);

    RuleId add_forward_rule(SwitchId src_switch_id, SwitchId dst_switch_id,
                            Priority priority, HeaderSpace domain);
    RuleId add_end_rule(SwitchId src_switch_id, Priority priority,
                        HeaderSpace header);

    DependencyGraph graph_;
    std::map<std::pair<SwitchId, SwitchId>, PortId> topology_;

    const TableId front_table = 0;

};
