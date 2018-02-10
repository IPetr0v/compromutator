#include "Detector.hpp"

#include <memory>

class Detector::Impl
{
public:
    explicit Impl(InstructionQueue& instruction_queue);

    void addSwitch(SwitchInfo info);
    void deleteSwitch(SwitchId id);

    void addRule(RuleInfo info);
    void deleteRule(RuleInfo info);

    void addLink(TopoId src_topo_id, TopoId dst_topo_id);
    void deleteLink(TopoId src_topo_id, TopoId dst_topo_id);

    void addRuleStats(RequestId xid, RuleStatsFields stats);
    void addPortStats(RequestId xid, PortStatsFields stats);

private:
    InstructionQueue& instruction_queue_;

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

Detector::Impl::Impl(InstructionQueue& instruction_queue):
    instruction_queue_(instruction_queue)
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

void Detector::Impl::addSwitch(SwitchInfo info)
{
    auto sw = network_->addSwitch(
        info.id, std::move(info.ports), info.table_number
    );

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

void Detector::Impl::deleteSwitch(SwitchId id)
{
    // TODO: critical - we lose statistics on switch deletion
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

void Detector::Impl::addRule(RuleInfo info)
{
    auto rule = network_->addRule(
        info.switch_id, info.table_id, info.priority,
        std::move(info.domain), std::move(info.actions)
    );
    add_rule_to_predictor(rule);

    execute_predictor_instruction();
}

void Detector::Impl::deleteRule(RuleInfo info)
{
    // TODO: delete rules by their domain and priority
    RuleId id;
    auto rule = network_->rule(id);
    delete_rule_from_predictor(rule);
    network_->deleteRule(id);

    execute_predictor_instruction();
}

void Detector::Impl::addLink(TopoId src_topo_id, TopoId dst_topo_id)
{
    auto link_pair = network_->addLink(src_topo_id, dst_topo_id);
    bool link_added = link_pair.second;
    if (link_added) {
        auto link = link_pair.first;
        add_link_to_predictor(link);

        execute_predictor_instruction();
    }
}

void Detector::Impl::deleteLink(TopoId src_topo_id, TopoId dst_topo_id)
{
    auto link_pair = network_->deleteLink(src_topo_id, dst_topo_id);
    bool link_exists = link_pair.second;
    if (link_exists) {
        auto link = link_pair.first;
        delete_link_from_predictor(link);

        execute_predictor_instruction();
    }
}

void Detector::Impl::addRuleStats(RequestId xid, RuleStatsFields stats)
{
    // TODO: check/find stats based on domain and port, not on xid
    auto it = pending_requests_.find(xid);
    if (it != pending_requests_.end()) {
        auto rule_request = RuleRequest::pointerCast(it->second);
        assert(rule_request != nullptr);
        rule_request->stats = stats;
        flow_predictor_->passRequest(rule_request);

        // TODO: change this
        //return StatsStatus::APPLIED;
    }
    else {
        // Stats weren't requested by the detector
        //return StatsStatus::DENIED;
    }
}

void Detector::Impl::addPortStats(RequestId xid, PortStatsFields stats)
{
    auto it = pending_requests_.find(xid);
    if (it != pending_requests_.end()) {
        auto port_request = PortRequest::pointerCast(it->second);
        assert(port_request != nullptr);
        port_request->stats = stats;
        flow_predictor_->passRequest(port_request);

        // TODO: change this
        //return StatsStatus::APPLIED;
    }
    else {
        // Stats weren't requested by the detector
        //return StatsStatus::DENIED;
    }
}

void Detector::Impl::execute_predictor_instruction()
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

void Detector::Impl::add_rule_to_predictor(RulePtr rule)
{
    auto diff = dependency_graph_->addRule(rule);
    flow_predictor_->updateEdges(diff);
}

void Detector::Impl::delete_rule_from_predictor(RulePtr rule)
{
    auto diff = dependency_graph_->deleteRule(rule);
    flow_predictor_->updateEdges(diff);
}

void Detector::Impl::add_link_to_predictor(Link link)
{
    auto diff = dependency_graph_->addLink(link);
    flow_predictor_->updateEdges(diff);
}

void Detector::Impl::delete_link_from_predictor(Link link)
{
    auto diff = dependency_graph_->deleteLink(link);
    flow_predictor_->updateEdges(diff);
}

Detector::Detector(std::shared_ptr<Alarm> alarm):
    instruction_queue_(alarm)
{
    impl_ = std::make_unique<Impl>(instruction_queue_);
}

Detector::~Detector()
{

}

void Detector::addSwitch(SwitchInfo info)
{
    executor_.addTask([this, info = std::move(info)]() {
        impl_->addSwitch(info);
    });
}

void Detector::deleteSwitch(SwitchId id)
{
    executor_.addTask([this, id]() {
        impl_->deleteSwitch(id);
    });
}

void Detector::addRule(RuleInfo info)
{
    executor_.addTask([this, info = std::move(info)]() {
        impl_->addRule(info);
    });
}

void Detector::deleteRule(RuleInfo info)
{
    executor_.addTask([this, info = std::move(info)]() {
        impl_->deleteRule(info);
    });
}

void Detector::addLink(TopoId src_topo_id, TopoId dst_topo_id)
{
    executor_.addTask([this, src_topo_id, dst_topo_id]() {
        impl_->addLink(src_topo_id, dst_topo_id);
    });
}

void Detector::deleteLink(TopoId src_topo_id, TopoId dst_topo_id)
{
    executor_.addTask([this, src_topo_id, dst_topo_id]() {
        impl_->deleteLink(src_topo_id, dst_topo_id);
    });
}

void Detector::addRuleStats(RequestId xid, RuleStatsFields stats)
{
    executor_.addTask([this, xid, stats]() {
        impl_->addRuleStats(xid, stats);
    });
}

void Detector::addPortStats(RequestId xid, PortStatsFields stats)
{
    executor_.addTask([this, xid, stats]() {
        impl_->addPortStats(xid, stats);
    });
}
