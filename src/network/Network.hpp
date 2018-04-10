#pragma once

#include "Rule.hpp"
#include "Switch.hpp"
#include "../openflow/Action.hpp"
#include "../Types.hpp"

#include <map>
#include <memory>
#include <vector>

struct Link
{
    PortPtr src_port;
    PortPtr dst_port;

    static Link inverse(Link link) {return {link.dst_port, link.src_port};}
};

class Network
{
public:
    Network();
    ~Network();

    // Switch management
    SwitchPtr getSwitch(SwitchId id) const;
    SwitchPtr addSwitch(const SwitchInfo& info);
    void deleteSwitch(SwitchId id);

    TablePtr getTable(SwitchId switch_id, TableId table_id) const;
    PortPtr getPort(TopoId topo_id) const;

    // Rule management
    RulePtr rule(RuleId id) const;
    RulePtr addRule(SwitchId switch_id, TableId table_id,
                    Priority priority, Cookie cookie,
                    NetworkSpace&& domain, ActionsBase&& actions_base);
    void deleteRule(RuleId id);

    PortPtr adjacentPort(PortPtr port) const;
    std::pair<Link, bool> link(TopoId src_topo_id, TopoId dst_topo_id);
    std::pair<Link, bool> addLink(TopoId src_topo_id, TopoId dst_topo_id);
    std::pair<Link, bool> deleteLink(TopoId src_topo_id, TopoId dst_topo_id);

    RulePtr dropRule() const {return drop_rule_;}
    RulePtr controllerRule() const {return controller_rule_;}

    // TODO: use RuleRange
    std::vector<RulePtr> rules();

private:
    std::map<SwitchId, SwitchPtr> switch_map_;
    // Topology keeps links as pairs {src_topo_id, dst_port_ptr}
    std::map<TopoId, PortPtr> topology_;

    // Special rules
    RulePtr drop_rule_;
    RulePtr controller_rule_;

    std::pair<bool, Actions> get_actions(SwitchPtr sw,
                                         ActionsBase&& actions_base) const;
    void add_rule_to_topology(RulePtr rule);
    void delete_rule_from_topology(RulePtr rule);

};
