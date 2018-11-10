#include "Detector.hpp"

#include <memory>

class Detector::Impl
{
public:
    explicit Impl(InstructionQueue& instruction_queue);

    void addSwitch(SwitchInfo&& info);
    void deleteSwitch(SwitchId id);

    void addRule(RuleInfo&& info);
    void changeRule(RuleInfo&& info);
    void deleteRule(RuleInfo&& info);

    void addLink(TopoId src_topo_id, TopoId dst_topo_id);
    void deleteLink(TopoId src_topo_id, TopoId dst_topo_id);

    void getRuleStats(RequestId request_id, RuleInfo&& info);
    void getPortStats(RequestId request_id, PortInfo&& info);

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

    RulePtr get_rule(const RuleInfo& info);
    std::list<RulePtr> get_matching_rules(const RuleInfo& info);
    void add_rule(RuleInfo&& info);
    void delete_rule(RulePtr rule);

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

    // Cleanup data structures
    network_->deleteSwitch(id);

    // Check network disconnection
    if (network_->empty()) {
        std::cout<<"[Detector] Network has been disconnected"<<std::endl;

        // Check dependency graph and path scan for emptiness
        assert(dependency_graph_->size() == 2);
        // TODO: Check FlowPredictor
    }
}

void Detector::Impl::addRule(RuleInfo&& info)
{
    auto rule = get_rule(info);
    if (not rule) {
        //DEBUG//std::cout<<"[Detector] FlowMod::ADD "<<info<<std::endl;
        add_rule(std::move(info));
    }
    else {
        // TODO: Check if the controller installs the same rule
        //std::cout<<"[Detector] FlowMod::CHANGE "<<info<<std::endl;
        //delete_rule(rule);
    }
}

void Detector::Impl::changeRule(RuleInfo&& info)
{
    //DEBUG//std::cout<<"[Detector] FlowMod::CHANGE "<<info<<std::endl;
    auto rule = get_rule(info);
    if (rule) {
        delete_rule(rule);
        add_rule(std::move(info));
    }
    else {
        throw std::logic_error("Change non-existing rule");
    }
}

void Detector::Impl::deleteRule(RuleInfo&& info)
{
    //DEBUG//std::cout<<"[Detector] FlowMod::DELETE "<<info<<std::endl;
    auto rules = get_matching_rules(info);
    for (const auto& rule : rules) {
        delete_rule(rule);
    }
    //auto rule = get_rule(info);
    //if (rule) {
    //    delete_rule(rule);
    //} else {
    //    throw std::logic_error("Delete non-existing rule");
    //}
}

void Detector::Impl::addLink(TopoId src_topo_id, TopoId dst_topo_id)
{
    auto link_pair = network_->addLink(src_topo_id, dst_topo_id);
    bool link_added = link_pair.second;
    if (link_added) {
        std::cout << "Link "
                  << src_topo_id.first << ":" << src_topo_id.second << " <-> "
                  << dst_topo_id.first << ":" << dst_topo_id.second
                  << " discovered" << std::endl;

        auto link = link_pair.first;
        add_link_to_predictor(link);
    }
}

void Detector::Impl::deleteLink(TopoId src_topo_id, TopoId dst_topo_id)
{
    auto link_pair = network_->deleteLink(src_topo_id, dst_topo_id);
    bool link_exists = link_pair.second;
    if (link_exists) {
        std::cout << "Link "
                  << src_topo_id.first << ":" << src_topo_id.second << " <-> "
                  << dst_topo_id.first << ":" << dst_topo_id.second
                  << " broken" << std::endl;

        auto link = link_pair.first;
        delete_link_from_predictor(link);
    }
}

void Detector::Impl::getRuleStats(RequestId request_id, RuleInfo&& info)
{
    auto rules = get_matching_rules(info);
    if (not rules.empty()) {
        flow_predictor_->predictFlow(request_id, rules);
    }
    else {
        std::cout << "[Detector] No matching rules" << std::endl;
    }
}

void Detector::Impl::getPortStats(RequestId request_id, PortInfo&& info)
{

}

void Detector::Impl::addRuleStats(RequestId request_id, RuleInfo&& info,
                                  RuleStatsFields stats)
{
    // TODO: Check this in StatsQuerier
    // TODO: check/find stats based on domain and port, not on xid
    auto it = pending_requests_.find(request_id);
    if (it != pending_requests_.end()) {
        auto rule_request = RuleRequest::pointerCast(it->second);
        assert(rule_request != nullptr);
        rule_request->stats = stats;
        flow_predictor_->passRequest(rule_request);

        pending_requests_.erase(it);

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

        pending_requests_.erase(it);

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
    // TODO: CRITICAL PERFORMANCE ISSUE - process path graph while preparing
    // instructions because sequential rule installation leads to large amount
    // of path computations!
    // Collect changed edges in diff and then use it it path computation.
    auto diff = dependency_graph_->popEdgeDiff();
    flow_predictor_->updateEdges(diff);
    auto instruction = flow_predictor_->getInstruction();
    // TODO: Save pending requests in Stats
    for (const auto& request : instruction.requests.data) {
        pending_requests_.emplace(request->id, request);
    }
    if (not instruction.empty()) {
        instruction_queue_.push(std::move(instruction));
    }
}

RulePtr Detector::Impl::get_rule(const RuleInfo& info)
{
    return network_->rule(
        info.switch_id, info.table_id, info.priority, info.match);
}

std::list<RulePtr> Detector::Impl::get_matching_rules(const RuleInfo& info)
{
    return network_->matchingRules(info.switch_id, info.table_id, info.match);
}

void Detector::Impl::add_rule(RuleInfo&& info)
{
    // TODO: add table miss only if there is a rule that sends packets
    // to this table, also return vector<rule> from addRule()
    auto rule = network_->addRule(
        info.switch_id, info.table_id, info.priority, info.cookie,
        std::move(info.match), std::move(info.actions)
    );
    add_rule_to_predictor(rule);
}

void Detector::Impl::delete_rule(RulePtr rule)
{
    delete_rule_from_predictor(rule);
    network_->deleteRule(rule->id());
}

void Detector::Impl::add_rule_to_predictor(RulePtr rule)
{
    //DEBUG//std::cout<<"[Graph] ADD "<<rule<<std::endl;
    dependency_graph_->addRule(rule);
    //flow_predictor_->updateEdges(diff);
}

void Detector::Impl::delete_rule_from_predictor(RulePtr rule)
{
    //DEBUG//std::cout<<"[Graph] DELETE "<<rule<<std::endl;
    dependency_graph_->deleteRule(rule);
    //flow_predictor_->updateEdges(diff);
}

void Detector::Impl::add_link_to_predictor(Link link)
{
    dependency_graph_->addLink(link);
    //flow_predictor_->updateEdges(diff);
}

void Detector::Impl::delete_link_from_predictor(Link link)
{
    dependency_graph_->deleteLink(link);
    //flow_predictor_->updateEdges(diff);
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

void Detector::addRule(RuleInfo info)
{
    executor_.addTask([this, info = std::move(info)]() mutable {
        impl_->addRule(std::move(info));
    });
}

void Detector::changeRule(RuleInfo info)
{
    executor_.addTask([this, info = std::move(info)]() mutable {
        impl_->changeRule(std::move(info));
    });
}

void Detector::deleteRule(RuleInfo info)
{
    executor_.addTask([this, info = std::move(info)]() mutable {
        impl_->deleteRule(std::move(info));
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

void Detector::getRuleStats(RequestId request_id, RuleInfo info)
{
    executor_.addTask(
        [this, request_id, info = std::move(info)]() mutable {
            impl_->getRuleStats(request_id, std::move(info));
        }
    );
}

void Detector::getPortStats(RequestId request_id, PortInfo info)
{
    executor_.addTask(
        [this, request_id, info = std::move(info)]() mutable {
            impl_->getPortStats(request_id, std::move(info));
        }
    );
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
