#include "Detector.hpp"

#include <memory>

class Detector::Impl
{
public:
    explicit Impl(InstructionQueue& instruction_queue);

    void addSwitch(SwitchInfo&& info);
    void deleteSwitch(SwitchId id);

    void addRule(SwitchId switch_id, RuleInfo&& info);
    void deleteRule(SwitchId switch_id, RuleInfo&& info);

    void addLink(TopoId src_topo_id, TopoId dst_topo_id);
    void deleteLink(TopoId src_topo_id, TopoId dst_topo_id);

    void addRuleStats(RequestId request_id, RuleInfo&& info,
                      RuleStatsFields stats);
    void addPortStats(RequestId request_id, PortInfo&& info,
                      PortStatsFields stats);

    void prepareInstructions();

private:
    InstructionQueue& instruction_queue_;

    // TODO: take away xids that are used by the controller
    std::shared_ptr<RequestIdGenerator> xid_generator_;

    std::shared_ptr<Network> network_;
    std::shared_ptr<DependencyGraph> dependency_graph_;
    std::unique_ptr<FlowPredictor> flow_predictor_;

    std::map<RequestId, RequestPtr> pending_requests_;

    void add_rule_to_predictor(RulePtr rule);
    void delete_rule_from_predictor(RulePtr rule);
    void add_link_to_predictor(Link link);
    void delete_link_from_predictor(Link link);

};

Detector::Impl::Impl(InstructionQueue& instruction_queue):
    instruction_queue_(instruction_queue)
{
    xid_generator_ = std::make_shared<RequestIdGenerator>();

    network_ = std::make_shared<Network>();
    dependency_graph_ = std::make_shared<DependencyGraph>(network_);
    flow_predictor_ = std::make_unique<FlowPredictor>(dependency_graph_,
                                                      xid_generator_);

    add_rule_to_predictor(network_->dropRule());
    add_rule_to_predictor(network_->controllerRule());
}

void Detector::Impl::addSwitch(SwitchInfo&& info)
{
    auto sw = network_->addSwitch(info);

    // Add special rules to the predictor
    for (auto port : sw->ports()) {
        add_rule_to_predictor(port->sourceRule());
        add_rule_to_predictor(port->sinkRule());
    }
    for (auto table : sw->tables()) {
        add_rule_to_predictor(table->tableMissRule());
    }
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
}

void Detector::Impl::addRule(SwitchId switch_id, RuleInfo&& info)
{
    NetworkSpace domain(std::move(info.match));
    auto rule = network_->addRule(
        switch_id, info.table_id, info.priority, info.cookie,
        std::move(domain), std::move(info.actions)
    );
    add_rule_to_predictor(rule);
}

void Detector::Impl::deleteRule(SwitchId switch_id, RuleInfo&& info)
{
    // TODO: delete rules by their domain and priority
    RuleId id;
    auto rule = network_->rule(id);
    if (rule) {
        delete_rule_from_predictor(rule);
        network_->deleteRule(id);
    }
}

void Detector::Impl::addLink(TopoId src_topo_id, TopoId dst_topo_id)
{
    auto link_pair = network_->addLink(src_topo_id, dst_topo_id);
    bool link_added = link_pair.second;
    if (link_added) {
        auto link = link_pair.first;
        add_link_to_predictor(link);

        std::cout << "Detector: new link "
                  << src_topo_id.first << ":" << src_topo_id.second << " <-> "
                  << dst_topo_id.first << ":" << dst_topo_id.second 
                  << std::endl;
    }
}

void Detector::Impl::deleteLink(TopoId src_topo_id, TopoId dst_topo_id)
{
    auto link_pair = network_->deleteLink(src_topo_id, dst_topo_id);
    bool link_exists = link_pair.second;
    if (link_exists) {
        auto link = link_pair.first;
        delete_link_from_predictor(link);
    }
}

void Detector::Impl::addRuleStats(RequestId request_id, RuleInfo&& info,
                                  RuleStatsFields stats)
{
    // TODO: check/find stats based on domain and port, not on xid
    auto it = pending_requests_.find(request_id);
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

void Detector::Impl::addPortStats(RequestId request_id, PortInfo&& info,
                                  PortStatsFields stats)
{
    auto it = pending_requests_.find(request_id);
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

void Detector::Impl::prepareInstructions()
{
    auto instruction = flow_predictor_->getInstruction();
    instruction_queue_.push(std::move(instruction));
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
    executor_.addTask([this, info = std::move(info)]() mutable {
        impl_->addSwitch(std::move(info));
    });
}

void Detector::deleteSwitch(SwitchId id)
{
    executor_.addTask([this, id]() {
        impl_->deleteSwitch(id);
    });
}

void Detector::addRule(SwitchId switch_id, RuleInfo info)
{
    executor_.addTask([this, switch_id, info = std::move(info)]() mutable {
        impl_->addRule(switch_id, std::move(info));
    });
}

void Detector::deleteRule(SwitchId switch_id, RuleInfo info)
{
    executor_.addTask([this, switch_id, info = std::move(info)]() mutable {
        impl_->deleteRule(switch_id, std::move(info));
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

void Detector::addRuleStats(RequestId request_id, RuleInfo info,
                            RuleStatsFields stats)
{
    executor_.addTask(
        [this, request_id, info = std::move(info), stats]() mutable {
            impl_->addRuleStats(request_id, std::move(info), stats);
        }
    );
}

void Detector::addPortStats(RequestId request_id, PortInfo info,
                            PortStatsFields stats)
{
    executor_.addTask(
        [this, request_id, info = std::move(info), stats]() mutable {
            impl_->addPortStats(request_id, std::move(info), stats);
        }
    );
}

void Detector::prepareInstructions()
{
    executor_.addTask([this]() {
        impl_->prepareInstructions();
    });
}
