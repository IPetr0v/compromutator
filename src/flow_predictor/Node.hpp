#pragma once

#include "Timestamp.hpp"

#include <cstdint>
#include <list>

using NodeId = uint64_t;
using PathId = uint64_t;

struct Node;
using NodePtr = std::list<Node>::iterator;
using NodeRemovalIterator = std::list<NodePtr>::iterator;

struct DomainPath;
using DomainPathPtr = std::list<DomainPath>::iterator;

struct RuleMapping
{
    RuleMapping(): counter(0), final_time(Timestamp::max()) {}
    bool empty() const {return node_list.empty();}

    uint64_t counter;
    std::list<NodePtr> node_list;
    Timestamp final_time;
};
using RuleMappingDescriptor = std::list<RuleMapping>::iterator;
