#pragma once

#include "../Common.hpp"
#include "../Graph.hpp"
#include "../dependency_graph/DependencyGraph.hpp"

#include <map>
#include <memory>

class Node;
using NodePtr = Node*;

using NodeGraph = Graph<Node>;
using NodeDescriptor = NodeGraph::VertexDescriptor;
using NodeRange = NodeGraph::VertexRange;
using NodeList = std::vector<NodeDescriptor>;

class Node
{
public:
    Node(RulePtr rule, NetworkSpace domain, uint16_t multiplier);
    explicit Node(Node&& other) noexcept;

    Node& operator=(Node&& other) noexcept;

    RulePtr rule;
    NetworkSpace domain;
    uint16_t multiplier;

    uint64_t counter;

    friend class PathScan;

private:
    NodeDescriptor descriptor_;
    bool is_interceptor_;
};

class PathScan
{
public:
    std::vector<NodeDescriptor> getNodes(RuleId id);
    std::vector<NodeDescriptor> getBaseNodes(RuleId id);
    NodeDescriptor addRootNode(RulePtr rule);
    NodeDescriptor addChildNode(NodeDescriptor parent, RulePtr rule,
                                NetworkSpace domain, uint16_t multiplier);
    NodeDescriptor addBaseNode(RulePtr rule, NetworkSpace domain);
    void deleteNode(NodeDescriptor node);

    Node& operator[](NodeDescriptor desc);
    NodeRange parentNodes(NodeDescriptor node);
    NodeRange childNodes(NodeDescriptor node);

    void addEdge(NodeDescriptor src_node_desc,
                 NodeDescriptor dst_node_desc);
    void deleteEdge(NodeDescriptor src_node_desc,
                    NodeDescriptor dst_node_desc);

private:
    NodeGraph node_graph_;
    std::map<RuleId, std::vector<NodeDescriptor>> node_map_;
    // Supported rule id -> base nodes
    std::map<RuleId, std::vector<NodeDescriptor>> base_node_map_;

    NodeDescriptor add_node(RulePtr rule, NetworkSpace domain,
                            uint16_t multiplier);
    void delete_from_node_map(NodeDescriptor old_node);
};
