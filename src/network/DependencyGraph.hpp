#pragma once

#include "Network.hpp"
#include "Rule.hpp"
#include "Vertex.hpp"

#include <memory>

struct EdgeDiff
{
    std::vector<EdgeDescriptor> new_edges;
    std::vector<EdgeDescriptor> changed_edges;
    std::vector<std::pair<RulePtr, RulePtr>> removed_edges;

    void clear() {
        new_edges.clear();
        changed_edges.clear();
        removed_edges.clear();
    }
};

class DependencyGraph
{
public:
    explicit DependencyGraph(std::shared_ptr<Network> network);

    EdgeDiff addRule(RulePtr rule);
    EdgeDiff deleteRule(RulePtr rule);

    EdgeDiff addLink(Link link);
    EdgeDiff deleteLink(Link link);

    EdgeRange outEdges(RulePtr rule) {return graph_->outEdges(rule->vertex_);}
    EdgeRange inEdges(RulePtr rule) {return graph_->inEdges(rule->vertex_);}
    const Edge& edge(EdgeDescriptor desc) const;

private:
    std::shared_ptr<Network> network_;
    std::unique_ptr<GraphData> graph_;
    EdgeDiff latest_diff_;

    VertexDescriptor add_vertex(RulePtr rule);
    void delete_vertex(RulePtr rule);

    void add_out_edges(RulePtr src_rule);
    void add_in_edges(RulePtr dst_rule);
    void delete_out_edges(RulePtr src_rule);
    void delete_in_edges(RulePtr dst_rule);

    void add_edges_to_port(RulePtr src_rule, const PortAction& action);
    void add_edges_to_table(RulePtr src_rule, const TableAction& action);
    void add_edges_to_group(RulePtr src_rule, const GroupAction& action);

    void add_edges(RulePtr src_rule, RuleRange dst_rules,
                   const Transfer& transfer, NetworkSpace output_domain);
    //void add_edges(RuleRange src_rules, RulePtr dst_rules);

    std::pair<Edge&, bool> get_edge();

    inline void add_edge(RulePtr src_rule, RulePtr dst_rule,
                         const Transfer& transfer, const NetworkSpace& domain);
    inline void delete_edge(RulePtr src_rule, RulePtr dst_rule);
    inline void set_edge_domain(EdgeDescriptor edge,
                                const NetworkSpace& domain);

};
