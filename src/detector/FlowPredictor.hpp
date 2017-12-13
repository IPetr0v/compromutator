#pragma once

#include "../Common.hpp"
#include "../Graph.hpp"
#include "../dependency_graph/DependencyGraph.hpp"
#include "PathScan.hpp"

#include <memory>

using InterceptorId = uint64_t;
class InterceptorManager
{
public:
    virtual bool installInterceptor(RulePtr interceptor_rule) = 0;
    virtual uint64_t getInterceptorCounter(InterceptorId id) const = 0;
};

// This struct defines changes in interceptor rules - additions and deletions
struct InterceptorEnvironment
{
    std::vector<RulePtr> rules_to_delete;
    std::vector<RulePtr> rules_to_add;
};

// This structure is a parameter to the getCounter function
// In order to predict counter values, the FlowPredictor needs statistics
// from a chosen number of interceptor rules
struct InterceptorStatistics
{
    std::vector<RulePtr> rules;
};

struct DependencyGraphDiff
{
    std::vector<RuleId> new_vertices;
    std::vector<std::pair<RuleId, RuleId>> new_out_edges;
    std::vector<std::pair<RuleId, RuleId>> new_in_edges;

    std::vector<RuleId> removed_vertices;
    std::vector<std::pair<RuleId, RuleId>> removed_out_edges;
    std::vector<std::pair<RuleId, RuleId>> removed_in_edges;

    std::vector<std::pair<RuleId, RuleId>> changed_edges;
};

struct Diff
{
    std::vector<std::pair<RuleId, RuleId>> new_edges;
    std::vector<std::pair<RuleId, RuleId>> changed_edges;
    std::vector<std::pair<RuleId, RuleId>> removed_edges;
};

struct NodeDiff
{

};

struct InterceptorDiff
{
    void merge(InterceptorDiff other);

    std::vector<RulePtr> rules_to_add;
    std::vector<RuleId> rules_to_delete;
};

class FlowPredictor
{
public:
    explicit FlowPredictor(DependencyGraph& dependency_graph);
    
    std::vector<RulePtr> addRule(RulePtr new_rule);
    InterceptorDiff updatePathScan(const Diff& diff);
    void deleteRule(RulePtr rule);

    uint64_t getCounter(RulePtr rule);

private:
    DependencyGraph& dependency_graph_;
    PathScan path_scan_;

    /*NodeGraph node_graph_;
    std::map<RuleId, std::vector<NodePtr>> node_map_;
    std::map<RuleId, uint64_t> counter_map_;*/

    std::vector<NodePtr> add_root(RulePtr rule);
    std::pair<NodeDescriptor, bool> add_child_node(NodeDescriptor parent,
                                                   NetworkSpace edge_domain,
                                                   RulePtr rule);
    void add_subtrees(std::pair<RuleId, RuleId> edge);
    void delete_subtrees(std::pair<RuleId, RuleId> edge);
    InterceptorDiff add_subtree(NodeDescriptor root);
    InterceptorDiff delete_subtree(NodeDescriptor root);

    NodePtr add_base(RulePtr rule);
    InterceptorDiff split_base_nodes(NodeDescriptor new_source_node);
    InterceptorDiff unite_base_nodes(NodeDescriptor old_source_node);

    NodePtr add_node(NodePtr parent, DependencyPtr dependency, RulePtr rule);

    void delete_nodes(RulePtr rule);

    void update_counter(NodePtr node);

};
