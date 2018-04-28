#pragma once

#include "Graph.hpp"
#include "../network/Types.hpp"
#include "../NetworkSpace.hpp"

class Rule;

struct Influence {
    Influence(RulePtr src_rule, RulePtr dst_rule, NetworkSpace domain):
        src_rule(src_rule), dst_rule(dst_rule), domain(domain) {}

    RulePtr src_rule;
    RulePtr dst_rule;
    NetworkSpace domain;
};

using InfluenceGraph = Graph<EmptyVertex, Influence>;
using InfluenceVertex = InfluenceGraph::VertexPtr;

struct Vertex {
    Vertex(RulePtr rule, NetworkSpace domain, InfluenceVertex influence):
        rule(rule), domain(domain), influence_vertex(influence) {}

    RulePtr rule;
    NetworkSpace domain;
    InfluenceVertex influence_vertex;
};

struct Edge {
    Edge(Transfer transfer, NetworkSpace domain):
        transfer(transfer), domain(domain) {}

    Transfer transfer;
    NetworkSpace domain;
};

using RuleGraph = Graph<Vertex, Edge>;
using VertexPtr = RuleGraph::VertexPtr;
using EdgePtr = RuleGraph::EdgePtr;
using EdgeRange = RuleGraph::EdgeRange;
