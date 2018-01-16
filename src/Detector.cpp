#include "Detector.hpp"

#include <memory>

Detector::Detector()
{
    controller_ = std::make_unique<Controller>();
    xid_generator_ = std::make_shared<RequestIdGenerator>();

    network_ = std::make_shared<Network>();
    dependency_graph_ = std::make_shared<DependencyGraph>(network_);
    flow_predictor_ = std::make_unique<FlowPredictor>(dependency_graph_,
                                                      xid_generator_);

    add_rule_to_predictor(network_->dropRule());
    add_rule_to_predictor(network_->controllerRule());
}

void Detector::addSwitch(SwitchId id, std::vector<PortId> ports,
                         uint8_t table_number)
{
    auto sw = network_->addSwitch(id, std::move(ports), table_number);

    // Add special rules to the predictor
    for (auto port : sw->ports()) {
        add_rule_to_predictor(port->sourceRule());
        add_rule_to_predictor(port->sinkRule());
    }
    for (auto table : sw->tables()) {
        add_rule_to_predictor(table->tableMissRule());
    }

    execute_predictor_instruction();
}

void Detector::deleteSwitch(SwitchId id)
{
    auto sw = network_->getSwitch(id);
    if (not sw) return;

    // Delete links from the predictor
    for (auto src_port : sw->ports()) {
        auto dst_port = network_->adjacentPort(src_port);
        if (dst_port) {
            delete_link_from_predictor({src_port, dst_port});
        }
    }

    // Delete rules from the predictor
    for (auto port : sw->ports()) {
        delete_rule_from_predictor(port->sourceRule());
        delete_rule_from_predictor(port->sinkRule());
    }
    for (auto table : sw->tables()) {
        for (auto rule : table->rules()) {
            delete_rule_from_predictor(rule);
        }
    }

    network_->deleteSwitch(id);

    execute_predictor_instruction();
}

void Detector::addRule(SwitchId switch_id, TableId table_id,
                       Priority priority, NetworkSpace domain,
                       ActionsBase actions)
{
    auto rule = network_->addRule(switch_id, table_id, priority,
                                  std::move(domain), std::move(actions));
    add_rule_to_predictor(rule);

    execute_predictor_instruction();
}

void Detector::deleteRule(RuleId id)
{
    auto rule = network_->rule(id);
    delete_rule_from_predictor(rule);
    network_->deleteRule(id);

    execute_predictor_instruction();
}

void Detector::addLink(TopoId src_topo_id, TopoId dst_topo_id)
{
    auto link_pair = network_->addLink(src_topo_id, dst_topo_id);
    bool link_added = link_pair.second;
    if (link_added) {
        auto link = link_pair.first;
        add_link_to_predictor(link);

        execute_predictor_instruction();
    }
}

void Detector::deleteLink(TopoId src_topo_id, TopoId dst_topo_id)
{
    auto link_pair = network_->link(src_topo_id, dst_topo_id);
    bool link_exists = link_pair.second;
    if (link_exists) {
        auto link = link_pair.first;
        delete_link_from_predictor(link);

        execute_predictor_instruction();
    }
}

StatsStatus Detector::addRuleStats(RequestId xid, RuleStatsFields stats)
{
    auto it = pending_requests_.find(xid);
    if (it != pending_requests_.end()) {
        auto rule_request = RuleRequest::pointerCast(it->second);
        assert(rule_request != nullptr);
        rule_request->stats = stats;
        flow_predictor_->passRequest(rule_request);

        return StatsStatus::APPLIED;
    }
    else {
        // Stats weren't requested by the detector
        return StatsStatus::DENIED;
    }
}

StatsStatus Detector::addPortStats(RequestId xid, PortStatsFields stats)
{
    auto it = pending_requests_.find(xid);
    if (it != pending_requests_.end()) {
        auto port_request = PortRequest::pointerCast(it->second);
        assert(port_request != nullptr);
        port_request->stats = stats;
        flow_predictor_->passRequest(port_request);

        return StatsStatus::APPLIED;
    }
    else {
        // Stats weren't requested by the detector
        return StatsStatus::DENIED;
    }
}

void Detector::execute_predictor_instruction()
{
    auto instruction = flow_predictor_->getInstruction();

    for (const auto& request : instruction.requests.data) {
        auto request_id = request->id;
        if (auto rule_request = RuleRequest::pointerCast(request)) {
            auto rule = rule_request->rule;
            pending_requests_.emplace(request_id, rule_request);
            controller_->getRuleStats(request_id, rule);
        }
        else if (auto port_request = PortRequest::pointerCast(request)) {
            auto port = port_request->port;
            pending_requests_.emplace(request_id, port_request);
            controller_->getPortStats(request_id, port);
        }
        else {
            assert(0);
        }
    }

    for (auto rule_to_delete : instruction.interceptor_diff.rules_to_delete) {
        controller_->deleteRule(rule_to_delete);
    }

    for (auto rule_to_add : instruction.interceptor_diff.rules_to_add) {
        controller_->installRule(rule_to_add);
    }
}

void Detector::add_rule_to_predictor(RulePtr rule)
{
    auto diff = dependency_graph_->addRule(rule);
    flow_predictor_->updateEdges(diff);
}

void Detector::delete_rule_from_predictor(RulePtr rule)
{
    auto diff = dependency_graph_->deleteRule(rule);
    flow_predictor_->updateEdges(diff);
}

void Detector::add_link_to_predictor(Link link)
{
    auto diff = dependency_graph_->addLink(link);
    flow_predictor_->updateEdges(diff);
}

void Detector::delete_link_from_predictor(Link link)
{
    auto diff = dependency_graph_->deleteLink(link);
    flow_predictor_->updateEdges(diff);
}
