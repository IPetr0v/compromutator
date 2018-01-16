#pragma once

#include "Node.hpp"
#include "Timestamp.hpp"
#include "../network/Graph.hpp"
#include "../network/DependencyGraph.hpp"
#include "../network/Network.hpp"
#include "../network/Rule.hpp"

#include <map>
#include <memory>

struct Node
{
    Node(NodeId id, RulePtr rule, NetworkSpace&& domain,
         Transfer root_transfer, uint64_t multiplier);
    Node(Node&& other) noexcept = default;

    Node& operator=(Node&& other) noexcept = default;
    const std::list<NodeDescriptor>& children() const {return children_;}

    NodeId id;
    NodeDescriptor parent;
    std::list<NodeDescriptor> children_;

    RulePtr rule;
    NetworkSpace domain;
    Transfer root_transfer;
    uint64_t multiplier;

    uint64_t addCounter(uint64_t new_counter) {
        rule_mapping->counter += new_counter;
        return counter += new_counter;
    }
    uint64_t counter;

    RuleMappingDescriptor rule_mapping;
    NodeRemovalIterator parent_backward_iterator_;
    NodeRemovalIterator vertex_backward_iterator_;
    Timestamp final_time_;
    DomainPathDescriptor out_path_;
};

struct DomainPath
{
    DomainPath(NodeDescriptor source, NodeDescriptor sink,
               Timestamp starting_time);

    NodeDescriptor source;
    NodeDescriptor sink;

    NetworkSpace source_domain;
    NetworkSpace sink_domain;
    RulePtr source_interceptor;
    RulePtr sink_interceptor;

    uint64_t last_counter;

    Timestamp starting_time;
    Timestamp final_time;
};

class PathScan
{
public:
    using NodeVisitor = std::function<void(NodeDescriptor node)>;
    using NodeDeletingVisitor = std::function<bool(NodeDescriptor node)>;

    PathScan(): last_node_id_(0) {}

    const Node& node(NodeDescriptor desc) const {return *desc;}
    const std::list<NodeDescriptor>& getNodes(RulePtr rule) const;
    uint64_t getRuleCounter(RulePtr rule) const {
        return rule->rule_mapping_->counter;
    }
    uint64_t addNodeCounter(NodeDescriptor node, uint64_t new_counter) {
        return node->addCounter(new_counter);
    }

    void forEachSubtreeNode(NodeDescriptor root, NodeVisitor visitor);
    void forEachPathNode(NodeDescriptor source, NodeDescriptor sink,
                         NodeDeletingVisitor deleting_visitor);

    NodeDescriptor addRootNode(RulePtr rule);
    NodeDescriptor addChildNode(NodeDescriptor parent, RulePtr rule,
                                NetworkSpace&& domain, const Transfer& transfer,
                                uint64_t multiplier);
    void setNodeFinalTime(NodeDescriptor node, Timestamp time);
    void deleteNode(NodeDescriptor node);

    const DomainPath& domainPath(DomainPathDescriptor desc) const;
    DomainPathDescriptor outDomainPath(NodeDescriptor source) const;
    DomainPathDescriptor addDomainPath(NodeDescriptor source,
                                       NodeDescriptor sink,
                                       Timestamp starting_time);
    void setDomainPathFinalTime(DomainPathDescriptor path, Timestamp time);
    void deleteDomainPath(DomainPathDescriptor path);

private:
    std::list<RuleMapping> rule_mapping_list_;
    std::list<Node> node_list_;
    std::list<DomainPath> domain_path_list_;
    NodeId last_node_id_;

    RuleMappingDescriptor add_rule_mapping(RulePtr rule);
    void delete_rule_mapping(RulePtr rule);

    NodeDescriptor add_node(RulePtr rule, NetworkSpace&& domain,
                            const Transfer& root_transfer,
                            uint64_t multiplier);

};
