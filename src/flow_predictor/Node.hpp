#pragma once

#include <cstdint>
#include <list>

using NodeId = uint64_t;
using PathId = uint64_t;

struct Node;
using NodePtr = Node*;
using NodeDescriptor = std::list<Node>::iterator;
using NodeRemovalIterator = std::list<NodeDescriptor>::iterator;

struct DomainPath;
using DomainPathDescriptor = std::list<DomainPath>::iterator;

struct RuleMapping
{
    bool empty() const {return node_list.empty();}

    uint64_t counter;
    std::list<NodeDescriptor> node_list;
};
using RuleMappingDescriptor = std::list<RuleMapping>::iterator;
