#pragma once

#include "Network.hpp"
#include "Rule.hpp"
#include "Vertex.hpp"
#include "EdgeDiff.hpp"

#include <memory>

class EdgeInstaller {
    struct EdgeData {
        VertexPtr src_vertex;
        VertexPtr dst_vertex;
        Transfer transfer;
        NetworkSpace domain;
        bool is_dependent;
    };
public:
    explicit EdgeInstaller(RuleGraph& graph);
    EdgeDiff popEdgeDiff();

    EdgePtr addEdge(VertexPtr src, VertexPtr dst,
                    Transfer transfer = Transfer::identityTransfer(),
                    bool is_dependent = false);
    void updateEdge(EdgePtr edge);
    void deleteEdge(VertexPtr src, VertexPtr dst, bool is_dependent = false);
    void deleteOutEdges(VertexPtr src, bool is_dependent = false);
    void deleteInEdges(VertexPtr dst, bool is_dependent = false);

    void clearEmptyEdges();

private:
    RuleGraph& rule_graph_;
    EdgeDiff diff_;
    std::vector<EdgePtr> empty_edges_;
};

class DependencyGraph
{
public:
    explicit DependencyGraph(std::shared_ptr<Network> network);

    void addRule(RulePtr rule);
    void deleteRule(RulePtr rule);

    void addLink(Link link);
    void deleteLink(Link link);

    EdgeDiff popEdgeDiff();

    EdgeRange outEdges(RulePtr rule) {return rule_graph_.outEdges(rule->vertex_);}
    EdgeRange inEdges(RulePtr rule) {return rule_graph_.inEdges(rule->vertex_);}

    const Vertex& vertex(RulePtr rule) const;
    const Edge& edge(EdgePtr edge) const;

private:
    std::shared_ptr<Network> network_;
    RuleGraph rule_graph_;
    InfluenceGraph influence_graph_;

    EdgeInstaller edge_installer_;
    EdgeDiff latest_diff_;

    VertexPtr add_vertex(RulePtr rule);
    void delete_vertex(RulePtr rule);

    void add_in_edges(RulePtr dst_rule);
    void delete_in_edges(RulePtr dst_rule);

    void add_out_edges(RulePtr src_rule);
    void add_edges_to_port(RulePtr src_rule, const PortAction& action);
    void add_edges_to_table(RulePtr src_rule, const TableAction& action);
    void add_edges_to_group(RulePtr src_rule, const GroupAction& action);
    void delete_out_edges(RulePtr src_rule);

    void add_edges(RulePtr src_rule, RuleRange dst_rules,
                   Transfer transfer = Transfer::identityTransfer());
    void update_domain(RulePtr rule);

    void add_influence(RulePtr src_rule, RulePtr dst_rule, NetworkSpace domain);
    void delete_influence(RulePtr src_rule, RulePtr dst_rule);

};
