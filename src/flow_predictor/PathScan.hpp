#pragma once

#include "Node.hpp"
#include "Timestamp.hpp"
#include "../network/Graph.hpp"
#include "../network/DependencyGraph.hpp"
#include "../network/Network.hpp"
#include "../network/Rule.hpp"

#include <functional>
#include <map>
#include <memory>

struct Node
{
    Node(NodeId id, RulePtr rule, NetworkSpace&& domain,
         Transfer root_transfer, uint64_t multiplier);
    Node(Node&& other) noexcept = default;

    Node& operator=(Node&& other) noexcept = default;

    NodeId id;
    NodePtr root;
    NodePtr parent;
    std::list<NodePtr> children_;

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
    DomainPathPtr out_path_;
};

struct DomainPath
{
    DomainPath(PathId id, NodePtr source, NodePtr sink,
               Timestamp starting_time);

    PathId id;
    NodePtr source;
    NodePtr sink;

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
    using NodeVisitor = std::function<void(NodePtr node)>;
    using NodeDeletingVisitor = std::function<bool(NodePtr node)>;

    PathScan(): last_node_id_(0), last_path_id_(0) {}

    bool ruleExists(RulePtr rule) const {
        return RuleMappingDescriptor(nullptr) != rule->rule_mapping_;
    }
    const Node& node(NodePtr desc) const {return *desc;}
    const std::list<NodePtr>& getNodes(RulePtr rule) const {
        return rule->rule_mapping_->node_list;
    }
    const std::list<NodePtr>& getChildNodes(NodePtr node) const {
        return node->children_;
    }
    uint64_t getRuleCounter(RulePtr rule) const {
        return rule->rule_mapping_->counter;
    }
    uint64_t addNodeCounter(NodePtr node, uint64_t new_counter) {
        return node->addCounter(new_counter);
    }

    void forEachSubtreeNode(NodePtr root, NodeVisitor visitor);
    void forEachPathNode(NodePtr source, NodePtr sink,
                         NodeDeletingVisitor deleting_visitor);

    NodePtr addRootNode(RulePtr rule);
    NodePtr addChildNode(NodePtr parent, RulePtr rule,
                                NetworkSpace&& domain, const Transfer& transfer,
                                uint64_t multiplier);
    void setNodeFinalTime(NodePtr node, Timestamp final_time);
    void deleteNode(NodePtr node);

    const DomainPath& domainPath(DomainPathPtr desc) const;
    DomainPathPtr outDomainPath(NodePtr source) const;
    DomainPathPtr addDomainPath(NodePtr source,
                                       NodePtr sink,
                                       Timestamp starting_time);
    void setDomainPathFinalTime(DomainPathPtr path,
                                Timestamp final_time);
    void deleteDomainPath(DomainPathPtr path);

private:
    std::list<RuleMapping> rule_mapping_list_;
    std::list<Node> node_list_;
    std::list<DomainPath> domain_path_list_;
    NodeId last_node_id_;
    PathId last_path_id_;

    RuleMappingDescriptor add_rule_mapping(RulePtr rule);
    void delete_rule_mapping(RulePtr rule);
    NodePtr add_node(RulePtr rule, NetworkSpace&& domain,
                            const Transfer& root_transfer,
                            uint64_t multiplier);

};
