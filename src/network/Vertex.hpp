#pragma once

#include "Graph.hpp"
#include "../network/Types.hpp"
#include "../NetworkSpace.hpp"

class Rule;

struct Vertex
{
    RulePtr rule;
};

struct Edge
{
    RulePtr src_rule;
    RulePtr dst_rule;
    Transfer transfer;
    NetworkSpace domain;
};

using GraphData = Graph<Vertex, Edge>;
using VertexDescriptor = GraphData::VertexDescriptor;
using EdgeDescriptor = GraphData::EdgeDescriptor;
using EdgeRange = GraphData::EdgeRange;
