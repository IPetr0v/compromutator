#include "FlowPredictor.hpp"

#include <algorithm>
#include <memory>
#include <queue>

InterceptorDiff& InterceptorDiff::operator+=(const InterceptorDiff& other)
{
    // Delete nonexistent rules
    auto it = std::remove_if(rules_to_add.begin(), rules_to_add.end(),
        [other](RulePtr rule) {
            for (const auto other_rule : other.rules_to_delete) {
                if (other_rule->id() == rule->id()) {
                 return true;
                }
            }
            return false;
        }
    );
    rules_to_add.erase(it, rules_to_add.end());

    rules_to_add.insert(rules_to_add.end(),
                        other.rules_to_add.begin(),
                        other.rules_to_add.end());
    rules_to_delete.insert(rules_to_delete.end(),
                           other.rules_to_delete.begin(),
                           other.rules_to_delete.end());
    return *this;
}

FlowPredictor::FlowPredictor(std::shared_ptr<DependencyGraph> dependency_graph,
                             std::shared_ptr<RequestIdGenerator> xid_generator):
    dependency_graph_(dependency_graph),
    path_scan_(std::make_unique<PathScan>()),
    stats_manager_(std::make_unique<StatsManager>(xid_generator))
{

}

Instruction FlowPredictor::getInstruction()
{
    auto instruction = Instruction{
        std::move(stats_manager_->getNewRequests()),
        std::move(latest_interceptor_diff_)
    };
    return std::move(instruction);
}

void FlowPredictor::passRequest(RequestPtr request)
{
    stats_manager_->passRequest(request);
    auto stats_list = stats_manager_->popStatsList();
    process_stats_list(std::move(stats_list));
}

void FlowPredictor::updateEdges(const EdgeDiff& edge_diff)
{
    for (auto& removed_edge : edge_diff.removed_edges) {
        delete_subtrees(removed_edge);
    }

    for (auto& changed_edge : edge_diff.changed_edges) {
        auto src_rule = dependency_graph_->edge(changed_edge).src_rule;
        auto dst_rule = dependency_graph_->edge(changed_edge).dst_rule;
        delete_subtrees({src_rule, dst_rule});
        add_subtrees(changed_edge);
    }

    for (auto& new_edge : edge_diff.new_edges) {
        add_subtrees(new_edge);
    }
}

void FlowPredictor::predictCounter(RulePtr rule)
{
    stats_manager_->requestRule(rule);
    auto nodes = path_scan_->getNodes(rule);
    for (auto node : nodes) {
        predict_subtree(node);
    }
}

void FlowPredictor::predict_subtree(NodeDescriptor root)
{
    path_scan_->forEachSubtreeNode(root, [this, root](NodeDescriptor node) {
        auto rule = path_scan_->node(node).rule;
        if (rule->type() == RuleType::SOURCE) {
            query_domain_path(node, root);
        }
    });
}

void FlowPredictor::process_stats_list(std::list<StatsPtr>&& stats_list)
{
    // TODO: use byte counters

    // Process statistics
    std::list<Prediction> new_predictions;
    for (const auto& stats : stats_list) {
        if (auto rule_stats = std::dynamic_pointer_cast<RuleStats>(stats)) {
            auto rule = rule_stats->rule;
            auto time = rule_stats->time;
            auto real_counter = rule_stats->stats_fields.packet_count;
            //process_rule_query(rule_stats);
            new_predictions.emplace_back(rule, time, real_counter, 0);
        }
        else if (auto path_stats = std::dynamic_pointer_cast<PathStats>(stats))
            process_path_query(path_stats);
        else if (auto link_stats = std::dynamic_pointer_cast<LinkStats>(stats))
            process_link_query(link_stats);
        else
            assert(0);
    }

    // Check rule counters
    for (auto& prediction : new_predictions) {
        auto rule = prediction.rule;
        prediction.predicted_counter = path_scan_->getRuleCounter(rule);
    }
    predictions_.splice(predictions_.end(), std::move(new_predictions));
}

void FlowPredictor::process_rule_query(const RuleStatsPtr& query)
{

}

void FlowPredictor::process_path_query(const PathStatsPtr& query)
{
    auto path = query->path;

    auto source_counter = query->source_stats_fields.packet_count;
    //auto sink_counter = query->sink_stats_fields.packet_count;
    auto last_counter = path_scan_->domainPath(path).last_counter;
    auto traversing_counter = source_counter - last_counter;

    auto source = path_scan_->domainPath(path).source;
    auto sink = path_scan_->domainPath(path).sink;
    path_scan_->forEachPathNode(source, sink,
        [this, traversing_counter](NodeDescriptor node) -> bool {
            path_scan_->addNodeCounter(node, traversing_counter);
            auto node_final_time = path_scan_->node(node).final_time_;
            return node_final_time < current_time();
        }
    );
}

void FlowPredictor::process_link_query(const LinkStatsPtr& query)
{

}

#include <iostream>
void FlowPredictor::add_subtrees(EdgeDescriptor edge)
{
    auto dst_rule = dependency_graph_->edge(edge).dst_rule;

    bool is_new_rule = not path_scan_->ruleExists(dst_rule);
    if (RuleType::SINK == dst_rule->type() && is_new_rule) {
        auto root = path_scan_->addRootNode(dst_rule);
        std::cout << "addRootNode: " << root->rule << std::endl;
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

void FlowPredictor::delete_subtrees(std::pair<RulePtr, RulePtr> edge)
{
    auto src_rule = edge.first;
    if (path_scan_->ruleExists(src_rule)) {
        auto nodes = path_scan_->getNodes(src_rule);
        for (auto node_it = nodes.begin(); node_it != nodes.end();) {
            delete_subtree(*(node_it++));
        }
    }
}

bool FlowPredictor::is_existing_child(NodeDescriptor parent,
                                      EdgeDescriptor edge) const
{
    auto src_rule = dependency_graph_->edge(edge).src_rule;
    for (auto child : path_scan_->getChildNodes(parent)) {
        auto child_rule = path_scan_->node(child).rule;
        if (src_rule->id() == child_rule->id()) {
            return true;
        }
    }
    return false;
}

std::pair<NodeDescriptor, bool>
FlowPredictor::add_child_node(NodeDescriptor parent, EdgeDescriptor edge)
{
    auto parent_domain = path_scan_->node(parent).domain;
    auto parent_multiplier = path_scan_->node(parent).multiplier;
    auto edge_domain = dependency_graph_->edge(edge).domain;

    auto rule = dependency_graph_->edge(edge).src_rule;
    auto& transfer = dependency_graph_->edge(edge).transfer;
    auto domain =
        transfer.inverse(edge_domain & parent_domain) & rule->domain();
    auto multiplier = rule->multiplier() * parent_multiplier;

    std::cout << "add_child_node:"
              << " rule=" << rule
              << " transfer=" << transfer
              << " domain=" << domain
              << std::endl;

    // Create node
    if (not domain.empty() && not is_existing_child(parent, edge)) {
        auto new_node = path_scan_->addChildNode(
            parent, rule, std::move(domain), transfer, multiplier
        );
        return std::make_pair(new_node, true);
    }
    else {
        return std::make_pair(NodeDescriptor(), false);
    }
}

void FlowPredictor::add_subtree(NodeDescriptor subtree_root)
{
    std::cout<<"add_subtree: "<<subtree_root->rule<<" | "
             <<subtree_root->domain<<std::endl;
    std::queue<NodeDescriptor> node_queue;
    node_queue.push(subtree_root);
    while (not node_queue.empty()) {
        auto& node = node_queue.front();
        auto rule = path_scan_->node(node).rule;

        // TODO: save path to getPort

        if (rule->type() != RuleType::SOURCE) {
            for (auto& edge : dependency_graph_->inEdges(rule)) {
                // Save child nodes to the path scan
                auto result = add_child_node(node, edge);
                auto success = result.second;
                if (success) {
                    auto child_node = result.first;
                    std::cout<<"    "<<child_node->rule<<" | "
                             <<child_node->domain<<std::endl;
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

void FlowPredictor::delete_subtree(NodeDescriptor subtree_root)
{
    std::cout<<"delete_subtree: "<<subtree_root->rule<<" | "
             <<subtree_root->domain<<std::endl;
    path_scan_->forEachSubtreeNode(subtree_root,
        [this, subtree_root](NodeDescriptor node) {
            // TODO: delete path from getPort

            // Set node to be an old version
            path_scan_->setNodeFinalTime(node, current_time());
            std::cout<<"    "<<node->rule<<" | "
                     <<node->domain<<std::endl;

            auto rule = path_scan_->node(node).rule;
            if (rule->type() == RuleType::SOURCE) {
                auto root = path_scan_->node(subtree_root).root;
                delete_domain_path(node, root);
            }
        }
    );
}

void FlowPredictor::query_domain_path(NodeDescriptor source,
                                      NodeDescriptor sink)
{
    auto path = path_scan_->addDomainPath(source, sink, current_time());
    auto path_id = path_scan_->domainPath(path).id;
    auto source_rule = path_scan_->node(source).rule;
    auto sink_rule = path_scan_->node(sink).rule;
    stats_manager_->requestPath(path_id, path, source_rule, sink_rule);
}

void FlowPredictor::add_domain_path(NodeDescriptor source,
                                    NodeDescriptor sink)
{
    auto path = path_scan_->addDomainPath(source, sink, current_time());
    auto source_rule = path_scan_->domainPath(path).source_interceptor;
    //auto sink_rule = path_scan_->domainPath(path).sink_interceptor;
    std::cout<<"    Add path | time="<<path->starting_time.id
             <<", "<<source->domain<<std::endl;

    // Add interceptors
    InterceptorDiff new_diff;
    new_diff.rules_to_add.push_back(source_rule);
    //new_diff.rules_to_add.push_back(sink_rule);
    latest_interceptor_diff_ += new_diff;
}

void FlowPredictor::delete_domain_path(NodeDescriptor source,
                                       NodeDescriptor sink)
{
    auto path = path_scan_->outDomainPath(source);
    auto path_id = path_scan_->domainPath(path).id;
    auto path_time = path_scan_->domainPath(path).starting_time;
    std::cout<<"    Delete path | time="<<path_time.id
             <<", "<<source->domain<<std::endl;
    if (path_time != current_time()) {
        path_scan_->setDomainPathFinalTime(path, current_time());
        auto source_rule = path_scan_->domainPath(path).source_interceptor;
        auto sink_rule = path_scan_->domainPath(path).sink_interceptor;
        stats_manager_->requestPath(path_id, path, source_rule, sink_rule);

        // Delete interceptors
        InterceptorDiff new_diff;
        new_diff.rules_to_delete.push_back(source_rule);
        //new_diff.rules_to_delete.push_back(sink_rule);
        latest_interceptor_diff_ += new_diff;
    }
    else {
        // Delete domain path because it hasn't produced any interceptor
        stats_manager_->discardPathRequest(path_id);
        path_scan_->deleteDomainPath(path);
    }
}
