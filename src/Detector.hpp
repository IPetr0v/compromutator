#pragma once

#include "openflow/Controller.hpp"
#include "network/Network.hpp"
#include "network/DependencyGraph.hpp"
#include "flow_predictor/FlowPredictor.hpp"

#include <memory>

struct RuleBase
{
    SwitchId switch_id;
    TableId table_id;
    Priority priority;
    NetworkSpace domain;
    ActionsBase actions;
};

enum class StatsStatus {APPLIED, DENIED};

class Detector
{
public:
    Detector();

    void addSwitch(SwitchId id, std::vector<PortId> ports,
                   uint8_t table_number);
    void deleteSwitch(SwitchId id);

    void addRule(SwitchId switch_id, TableId table_id,
                 Priority priority, NetworkSpace domain,
                 ActionsBase actions);
    void deleteRule(RuleId id);

    void addLink(TopoId src_topo_id, TopoId dst_topo_id);
    void deleteLink(TopoId src_topo_id, TopoId dst_topo_id);

    StatsStatus addRuleStats(RequestId xid, RuleStatsFields stats);
    StatsStatus addPortStats(RequestId xid, PortStatsFields stats);

private:
    std::unique_ptr<Controller> controller_;
    // TODO: take away xids that are used by the controller
    std::shared_ptr<RequestIdGenerator> xid_generator_;

    std::shared_ptr<Network> network_;
    std::shared_ptr<DependencyGraph> dependency_graph_;
    std::unique_ptr<FlowPredictor> flow_predictor_;

    std::map<RequestId, RequestPtr> pending_requests_;

    void execute_predictor_instruction();

    void add_rule_to_predictor(RulePtr rule);
    void delete_rule_from_predictor(RulePtr rule);
    void add_link_to_predictor(Link link);
    void delete_link_from_predictor(Link link);

};
