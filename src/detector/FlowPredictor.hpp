#pragma once

#include "../Common.hpp"
#include "../Graph.hpp"
#include "../dependency_graph/DependencyGraph.hpp"
#include "PathScan.hpp"

#include <memory>

// This structure is a parameter to the getCounter function
// In order to predict counter values, the FlowPredictor needs statistics
// from a chosen number of interceptor rules
struct InterceptorStatistics
{
    std::vector<RulePtr> rules;
};

struct InterceptorDiff
{
    InterceptorDiff& operator+=(const InterceptorDiff& other);

    std::vector<RulePtr> rules_to_add;
    std::vector<RuleId> rules_to_delete;
};

class FlowPredictor
{
public:
    explicit FlowPredictor(DependencyGraph& dependency_graph);

    InterceptorDiff update(const DependencyDiff& dependency_diff);
    uint64_t getCounter(RulePtr rule);

    friend class Test;

private:
    DependencyGraph& dependency_graph_;
    PathScan path_scan_;

    std::vector<NodePtr> add_root(RulePtr rule);
    std::pair<NodeDescriptor, bool> add_child_node(NodeDescriptor parent,
                                                   NetworkSpace edge_domain,
                                                   RulePtr rule);
    InterceptorDiff add_subtrees(std::pair<RuleId, RuleId> edge);
    InterceptorDiff delete_subtrees(std::pair<RuleId, RuleId> edge);
    InterceptorDiff add_subtree(NodeDescriptor root);
    InterceptorDiff delete_subtree(NodeDescriptor root);

    NodePtr add_base(RulePtr rule);
    InterceptorDiff split_base_nodes(NodeDescriptor new_source_node);
    InterceptorDiff unite_base_nodes(NodeDescriptor old_source_node);

    NodePtr add_node(NodePtr parent, DependencyPtr dependency, RulePtr rule);

    void delete_nodes(RulePtr rule);

    void update_counter(NodePtr node);

};
