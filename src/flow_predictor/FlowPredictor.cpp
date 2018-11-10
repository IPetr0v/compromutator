#include "FlowPredictor.hpp"

#include <algorithm>
#include <memory>
#include <queue>

void Prediction::update(RuleStatsFields real, RuleStatsFields predicted)
{
    real_counter = real;
    predicted_counter = predicted;
}

FlowPredictor::FlowPredictor(std::shared_ptr<DependencyGraph> dependency_graph,
                             std::shared_ptr<RequestIdGenerator> xid_generator):
    dependency_graph_(dependency_graph),
    path_scan_(std::make_unique<PathScan>()),
    interceptor_manager_(std::make_unique<InterceptorManager>()),
    stats_manager_(std::make_unique<StatsManager>(xid_generator))
{

}

Instruction FlowPredictor::getInstruction()
{
    path_scan_->clearDeletedNodes();
    auto instruction = Instruction{
        stats_manager_->getNewRequests(),
        create_replies(),
        interceptor_manager_->popInterceptorDiff()
    };
    return instruction;
}

void FlowPredictor::passRequest(RequestPtr request)
{
    auto rule_request = RuleRequest::pointerCast(request);
    //std::cout<<"passRequest["<<request->time.id<<
    //         "] "<<*(rule_request->rule.get())<<std::endl;
    stats_manager_->passRequest(request);
    auto stats_list = stats_manager_->popStatsList();
    if (not stats_list.stats.empty()) {
        process_stats_list(std::move(stats_list.stats));
        update_predictions(request->time.id);
    }
}

void FlowPredictor::updateEdges(const EdgeDiff& edge_diff)
{
    if (not edge_diff.empty()) {
        std::cout << "[Graph] " << edge_diff
                  << " | " << interceptor_manager_->diffToString() << std::endl;
    }

    //std::cout<<"[Graph] Remove"<<std::endl;
    for (auto& removed_edge : edge_diff.removed_edges) {
        delete_subtrees(removed_edge);
    }

    //std::cout<<"[Graph] Change"<<std::endl;
    for (auto& changed_edge : edge_diff.changed_edges) {
        delete_subtrees(changed_edge);
        add_subtrees(changed_edge);
    }

    //std::cout<<"[Graph] Add"<<std::endl;
    for (auto& new_edge : edge_diff.new_edges) {
        add_subtrees(new_edge);
    }

    // TODO: uncomment this
    //if (not edge_diff.empty()) {
    //    std::cout << "[Graph] " << edge_diff
    //              << " | " << interceptor_manager_->diffToString() << std::endl;
    //}
}

void FlowPredictor::predictFlow(RequestId request_id, std::list<RulePtr> rules)
{
    for (const auto& rule : rules) {
        // Create pending predictions
        //DEBUG//std::cout<<"Add pred: "<<current_time().id<<std::endl;
        pending_predictions_[current_time().id].emplace_back(request_id, rule);

        // TODO: Get rule stats for detection
        //stats_manager_->requestRule(rule);

        // Create requests
        auto nodes = path_scan_->getNodes(rule);
        for (auto node : nodes) {
            predict_subtree(node);
        }
    }
}

void FlowPredictor::predict_subtree(NodePtr root)
{
    path_scan_->forEachSubtreeNode(root, [this, root](NodePtr node) {
        auto rule = path_scan_->node(node).rule;
        if (rule->type() == RuleType::SOURCE) {
            query_domain_path(node, root);
        }
    });
}

void FlowPredictor::update_predictions(TimestampId timestamp)
{
    auto it = pending_predictions_.find(timestamp);
    if (it != pending_predictions_.end()) {
        for (auto& prediction : it->second) {
            // TODO: Get real counter values
            auto rule = prediction.rule;
            prediction.update({0, 0}, path_scan_->getRuleCounter(rule));
            predictions_.push_back(std::move(prediction));
        }
        pending_predictions_.erase(it);
    }
    else {
        //throw std::logic_error("Non-existing prediction");
        //std::cout<<"Non-existing prediction: "<<timestamp<<std::endl;
    }
}

RuleReplyList FlowPredictor::create_replies()
{
    RuleReplyList replies;
    std::unordered_map<RequestId, RuleReplyPtr> reply_map;
    for (const auto& prediction : predictions_) {
        auto it = reply_map.find(prediction.request_id);
        if (it != reply_map.end()) {
            it->second->addFlow(prediction.rule->info(),
                                prediction.predicted_counter);
        }
        else {
            auto reply = std::make_shared<RuleReply>(
                prediction.request_id, prediction.rule->sw()->id()
            );
            reply->addFlow(prediction.rule->info(),
                           prediction.predicted_counter);
            reply_map.emplace(prediction.request_id, reply);
        }
    }
    predictions_.clear();
    for (const auto& reply : reply_map) {
        replies.push_back(reply.second);
    }
    return replies;
}

void FlowPredictor::process_stats_list(std::list<StatsPtr>&& stats_list)
{
    // TODO: use byte counters

    // Process statistics
    //std::list<Prediction> new_predictions;
    for (const auto& stats : stats_list) {
        if (auto rule_stats = std::dynamic_pointer_cast<RuleStats>(stats)) {
            assert(false);
            //auto rule = rule_stats->rule;
            //auto time = rule_stats->time;
            //auto real_counter = rule_stats->stats_fields;
            ////process_rule_query(rule_stats);
            //new_predictions.emplace_back(rule, time, real_counter,
            //                             RuleStatsFields{0, 0});
        }
        else if (auto path_stats = std::dynamic_pointer_cast<PathStats>(stats))
            process_path_query(path_stats);
        else if (auto link_stats = std::dynamic_pointer_cast<LinkStats>(stats))
            process_link_query(link_stats);
        else
            assert(false);
    }

    // Check rule counters
    //for (auto& prediction : new_predictions) {
    //    auto rule = prediction.rule;
    //    prediction.predicted_counter = path_scan_->getRuleCounter(rule);
    //}
    //predictions_.splice(predictions_.end(), std::move(new_predictions));
}

void FlowPredictor::process_rule_query(const RuleStatsPtr& query)
{

}

void FlowPredictor::process_path_query(const PathStatsPtr& query)
{
    auto path = query->path;

    auto source_counter = query->source_stats_fields;
    //auto sink_counter = query->sink_stats_fields.packet_count;
    auto last_counter = path->last_counter;
    auto packet_count = source_counter.packet_count - last_counter.packet_count;
    auto byte_count = source_counter.byte_count - last_counter.byte_count;
    RuleStatsFields traversing_counter{packet_count, byte_count};

    auto source = path_scan_->domainPath(path).source;
    auto sink = path_scan_->domainPath(path).sink;
    path_scan_->forEachPathNode(source, sink,
        [this, traversing_counter](NodePtr node) -> bool {
            path_scan_->addNodeCounter(node, traversing_counter);
            auto node_final_time = path_scan_->node(node).final_time_;
            return node_final_time < current_time();
        }
    );

    // Update last counter
    path->last_counter = source_counter;
}

void FlowPredictor::process_link_query(const LinkStatsPtr& query)
{

}

void FlowPredictor::add_subtrees(const Dependency& edge)
{
    auto dst_rule = edge.dst;

    bool is_new_rule = not path_scan_->ruleExists(dst_rule);
    if (RuleType::SINK == dst_rule->type() && is_new_rule) {
        path_scan_->addRootNode(dst_rule);
    }

    if (path_scan_->ruleExists(dst_rule)) {
        auto dst_nodes = path_scan_->getNodes(dst_rule);
        for (auto dst_node : dst_nodes) {
            auto result = add_child_node(dst_node, edge);
            auto success = result.second;
            if (success) {
                auto new_child = result.first;
                add_subtree(new_child);
            }
        }
    }
}

void FlowPredictor::delete_subtrees(const Dependency& edge)
{
    //std::cout<<"[Path] delete_subtrees "<<edge.first<<std::endl;
    auto src_rule = edge.src;
    if (path_scan_->ruleExists(src_rule)) {
        auto nodes = path_scan_->getNodes(src_rule);
        for (auto node_it = nodes.begin(); node_it != nodes.end();) {
            // Check correct edge
            auto node = *node_it;
            //std::cout << "[Path] --- node " << **node_it << std::endl;
            if (node->parent->rule->id() == edge.dst->id()) {
                delete_subtree(*node_it);
            }
            node_it++;
        }
    }
}

bool FlowPredictor::is_existing_child(NodePtr parent,
                                      const Dependency& edge) const
{
    auto src_rule = edge.src;
    for (auto child : path_scan_->getChildNodes(parent)) {
        auto child_rule = path_scan_->node(child).rule;
        if (src_rule->id() == child_rule->id()) {
            return true;
        }
    }
    return false;
}

std::pair<NodePtr, bool>
FlowPredictor::add_child_node(NodePtr parent, const Dependency& edge)
{
    auto parent_domain = path_scan_->node(parent).domain;
    auto parent_multiplier = path_scan_->node(parent).multiplier;
    //auto edge_domain = dependency_graph_->edge(edge).domain;
    auto edge_domain = edge.domain;

    //auto rule = edge->src->rule;
    auto rule = edge.src;
    auto& transfer = edge.transfer;
    auto domain =
        transfer.inverse(edge_domain & parent_domain) & rule->domain();
    auto multiplier = rule->multiplier() * parent_multiplier;

    // Create node
    if (not domain.empty() && not is_existing_child(parent, edge)) {
        auto new_node = path_scan_->addChildNode(
            parent, rule, std::move(domain), transfer, multiplier
        );
        return std::make_pair(new_node, true);
    }
    else {
        return std::make_pair(NodePtr(), false);
    }
}

void FlowPredictor::add_subtree(NodePtr subtree_root)
{
    //DEBUG//std::cout<<"add_subtree: "<<*subtree_root<<std::endl;
    std::queue<NodePtr> node_queue;
    node_queue.push(subtree_root);
    while (not node_queue.empty()) {
        auto& node = node_queue.front();
        //DEBUG//std::cout<<"-> "<<*node<<std::endl;
        auto rule = path_scan_->node(node).rule;

        // TODO: save path to getPort

        if (rule->type() != RuleType::SOURCE) {
            for (auto& edge : dependency_graph_->inEdges(rule)) {
                // Save child nodes to the path scan
                auto dependency = Dependency(edge);
                auto result = add_child_node(node, dependency);
                auto success = result.second;
                if (success) {
                    auto child_node = result.first;
                    node_queue.push(child_node);
                }
            }
        }
        else {
            auto root = path_scan_->node(subtree_root).root;
            add_domain_path(node, root);
        }
        node_queue.pop();
    }
}

void FlowPredictor::delete_subtree(NodePtr subtree_root)
{
    //DEBUG//std::cout<<"delete_subtree: "<<*subtree_root<<std::endl;
    path_scan_->forEachSubtreeNode(subtree_root,
        [this, subtree_root](NodePtr node) {
            //DEBUG//std::cout<<"-> "<<*node<<std::endl;
            // TODO: delete path from getPort

            // Set node to be an old version
            path_scan_->setNodeFinalTime(node, current_time());

            auto rule = path_scan_->node(node).rule;
            if (rule->type() == RuleType::SOURCE) {
                auto root = path_scan_->node(subtree_root).root;
                delete_domain_path(node, root);
            }
        }
    );
}

void FlowPredictor::query_domain_path(NodePtr source, NodePtr sink)
{
    auto path = path_scan_->outDomainPath(source);
    stats_manager_->requestPath(path);
    //DEBUG//std::cout<<"[Path] Query: "<<*path->interceptor<<std::endl;
}

void FlowPredictor::add_domain_path(NodePtr source, NodePtr sink)
{
    auto path = path_scan_->addDomainPath(source, sink, current_time());
    interceptor_manager_->createInterceptor(path);
    //DEBUG//std::cout<<"[Path] Create: "<<*path->interceptor<<std::endl;
}

void FlowPredictor::delete_domain_path(NodePtr source, NodePtr sink)
{
    auto path = path_scan_->outDomainPath(source);
    auto path_time = path_scan_->domainPath(path).starting_time;
    if (path_time != current_time()) {
        //DEBUG//std::cout<<"[Path] Suspend: "<<*path->interceptor<<std::endl;
        path_scan_->setDomainPathFinalTime(path, current_time());
        stats_manager_->requestPath(path);
        interceptor_manager_->deleteInterceptor(path);
    }
    else {
        // Delete domain path because it hasn't produced any interceptor
        if (path->interceptor) {
            //DEBUG//std::cout<<"[Path] Delete: "<<*path->interceptor<<std::endl;
            interceptor_manager_->deleteInterceptor(path);
        }
        else {
            //DEBUG//std::cout<<"[Path] Delete: "<<"[SOURCE: SUSPENDED]"<<std::endl;
        }
        stats_manager_->discardPathRequest(path);
        path_scan_->deleteDomainPath(path);

        // TODO: delete domain path from latest_interceptor_diff_
    }
}
