#include "FlowPredictor.hpp"

#include <algorithm>
#include <queue>

InterceptorDiff& InterceptorDiff::operator+=(const InterceptorDiff& other)
{
    // Delete nonexisting rules
    auto it = std::remove_if(rules_to_add.begin(),
                             rules_to_add.end(),
                             [other](RulePtr rule) {
                                 for (const auto id : other.rules_to_delete) {
                                     if (id == rule->id()) {
                                         return true;
                                     }
                                 }
                                 return false;
                             });
    rules_to_add.erase(it, rules_to_add.end());

    rules_to_add.insert(rules_to_add.end(),
                        other.rules_to_add.begin(),
                        other.rules_to_add.end());
    rules_to_delete.insert(rules_to_delete.end(),
                           other.rules_to_delete.begin(),
                           other.rules_to_delete.end());
    return *this;
}

FlowPredictor::FlowPredictor(DependencyGraph& dependency_graph):
    dependency_graph_(dependency_graph)
{
    
}

InterceptorDiff FlowPredictor::update(const DependencyDiff& dependency_diff)
{
    InterceptorDiff diff;
    for (const auto& removed_edge : dependency_diff.removed_edges) {
        diff += delete_subtrees(removed_edge);
    }

    for (const auto& changed_edge : dependency_diff.changed_edges) {
        diff += delete_subtrees(changed_edge);
        diff += add_subtrees(changed_edge);
    }

    for (const auto& new_edge : dependency_diff.new_edges) {
        diff += add_subtrees(new_edge);
    }
    return diff;
}

InterceptorDiff FlowPredictor::add_subtrees(std::pair<RuleId, RuleId> edge)
{
    InterceptorDiff diff;
    auto dst_rule_id = edge.second;
    auto dst_nodes = path_scan_.getNodes(dst_rule_id);
    auto new_edge_ptr = dependency_graph_.getDependency(edge.first,
                                                        edge.second);
    for (auto dst_node : dst_nodes) {
        auto result = add_child_node(dst_node, new_edge_ptr->domain,
                                     new_edge_ptr->src_rule);
        auto success = result.second;
        if (success) {
            auto new_child = result.first;
            diff += add_subtree(new_child);
        }
    }
    return diff;
}

InterceptorDiff FlowPredictor::delete_subtrees(std::pair<RuleId, RuleId> edge)
{
    InterceptorDiff diff;
    auto src_rule_id = edge.first;
    auto nodes_to_delete = path_scan_.getNodes(src_rule_id);
    for (const auto& node : nodes_to_delete) {
        diff += delete_subtree(node);
    }
    return diff;
}

std::pair<NodeDescriptor, bool>
FlowPredictor::add_child_node(NodeDescriptor parent,
                              NetworkSpace edge_domain,
                              RulePtr rule)
{
    auto& parent_node = path_scan_[parent];
    // TODO: Delete this temporary solution
    auto actions = rule->actions();
    auto domain = (!actions.empty())
                  ? actions[0].transfer.inverse(edge_domain &
                                                parent_node.domain)
                  : edge_domain;
    auto multiplier = rule->multiplier() * parent_node.multiplier;

    // Create node
    if (not domain.empty()) {
        auto new_node = path_scan_.addChildNode(parent, rule,
                                                domain, multiplier);
        return {new_node, true};
    } else {
        return {NodeDescriptor(), false};
    }
}

InterceptorDiff FlowPredictor::add_subtree(NodeDescriptor root)
{
    InterceptorDiff diff;

    std::queue<NodeDescriptor> node_queue;
    node_queue.push(root);
    while (not node_queue.empty()) {
        auto& parent_node = node_queue.front();
        auto parent_rule = path_scan_[parent_node].rule;

        // TODO: check for infinite cycles!!!
        // Add child nodes to the path scan
        for (const auto& edge : dependency_graph_.inDependencies(parent_rule)) {
            auto child_rule = edge->src_rule;

            auto result = add_child_node(parent_node, edge->domain, child_rule);
            auto success = result.second;
            if (success) {
                auto new_child = result.first;
                if (child_rule->type() != RuleType::SOURCE) {
                    node_queue.push(new_child);
                }
                else {
                    diff += split_base_nodes(new_child);
                }
            }
        }
        node_queue.pop();
    }

    return diff;
}

InterceptorDiff FlowPredictor::delete_subtree(NodeDescriptor root)
{
    InterceptorDiff diff;

    std::queue<NodeDescriptor> node_queue;
    node_queue.push(root);
    while (not node_queue.empty()) {
        auto& parent_node = node_queue.front();
        auto parent_rule = path_scan_[parent_node].rule;

        // Save child nodes to the queue
        if (parent_rule->type() != RuleType::SOURCE) {
            for (auto child : path_scan_.childNodes(parent_node)) {
                auto child_rule = path_scan_[child].rule;
                if (child_rule->type() != RuleType::SOURCE) {
                    node_queue.push(child);
                } else {
                    diff += unite_base_nodes(child);
                }
            }
        }

        // Delete parent node
        path_scan_.deleteNode(parent_node);
        node_queue.pop();
    }

    return diff;
}

InterceptorDiff FlowPredictor::split_base_nodes(NodeDescriptor new_source_node)
{
    InterceptorDiff diff;

    auto source_rule = path_scan_[new_source_node].rule;
    assert(source_rule->type() == RuleType::SOURCE);

    auto base_nodes = path_scan_.getBaseNodes(source_rule->id());
    for (auto base_node : base_nodes) {
        auto domain = path_scan_[base_node].domain &
                      path_scan_[new_source_node].domain;
        if (not domain.empty()) {
            auto diff_domain = path_scan_[base_node].domain -
                               path_scan_[new_source_node].domain;
            // New node occupies the whole domain
            if (diff_domain.empty()) {
                path_scan_.addEdge(base_node, new_source_node);
            }
            else {
                auto old_rule = path_scan_[base_node].rule;
                auto old_parent_nodes = path_scan_.parentNodes(base_node);

                // Add new base nodes
                auto old_changed_rule = std::make_shared<Rule>(RuleType::SOURCE,
                                                               old_rule->switchId(),
                                                               0, 0,
                                                               diff_domain,
                                                               old_rule->actions());
                diff.rules_to_add.push_back(old_changed_rule);
                auto changed_base_node = path_scan_.addBaseNode(
                    old_changed_rule, diff_domain
                );

                auto new_rule = std::make_shared<Rule>(RuleType::SOURCE,
                                                       old_rule->switchId(),
                                                       0, 0, domain,
                                                       old_rule->actions());
                diff.rules_to_add.push_back(new_rule);
                auto new_base_node = path_scan_.addBaseNode(new_rule, domain);

                // Add base node links
                for (auto old_parent_node : old_parent_nodes) {
                    path_scan_.addEdge(changed_base_node, old_parent_node);
                    path_scan_.addEdge(new_base_node, old_parent_node);
                }
                path_scan_.addEdge(new_base_node, new_source_node);

                // Delete old base node
                diff.rules_to_delete.push_back(old_rule->id());
                path_scan_.deleteNode(base_node);
            }
        }
    }

    return diff;
}

InterceptorDiff FlowPredictor::unite_base_nodes(NodeDescriptor old_source_node)
{
    InterceptorDiff diff;

    auto source_rule = path_scan_[old_source_node].rule;
    auto base_nodes = path_scan_.getBaseNodes(source_rule->id());
    auto old_base_nodes = path_scan_.childNodes(old_source_node);
    for (auto old_base_node : old_base_nodes) {
        // TODO: implement base node union

        auto old_rule = path_scan_[old_base_node].rule;
        diff.rules_to_delete.push_back(old_rule->id());
    }

    return diff;
}

void FlowPredictor::delete_nodes(RulePtr rule)
{


    /*
    if (rule->type() == RuleType::SINK) {
        auto new_node = add_node(nullptr, nullptr, rule);
        if (new_node) {
            add_subtree(new_node);
        }
    }
    else {
        for (auto dependency : dependency_graph_.outDependencies(rule)) {
            auto dst_rule = dependency->dst_rule;

            // If the destination rule has been already added
            auto it = node_map_.find(dst_rule->id());
            if (it != node_map_.end()) {
                auto dst_nodes = it->second;
                for (auto dst_node : dst_nodes) {
                    auto new_node = add_node(dst_node, dependency, rule);
                    if (new_node) {
                        if (dst_rule->type() == RuleType::SOURCE) {
                            add_base(new_node);
                        }
                        else {
                            add_subtree(new_node);
                        }
                    }
                }
            }
        }
    }
    */
}

uint64_t FlowPredictor::getCounter(RulePtr rule)
{
    /*uint64_t counter = 0;
    auto it = node_map_.find(rule->id());
    if (it == node_map_.end()) {
        auto nodes = it->second;
        for (auto node : nodes) {
            update_counter(node);
            counter += node->counter;
        }
    }
    return counter;*/
    return 0;
}

void FlowPredictor::update_counter(NodePtr node)
{

}
