#pragma once

#include "../Common.hpp"
#include "../NetworkSpace.hpp"

#include <memory>
#include <vector>

class Rule;
class Dependency;

class Action;

class DependencyUpdater;

typedef std::shared_ptr<Rule> RulePtr;
typedef std::shared_ptr<Dependency> DependencyPtr;

class Rule
{
public:
    Rule(SwitchId switch_id, TableId table_id, RuleId id,
         uint16_t priority, NetworkSpace& match,
         std::vector<Action>& action_list);
    
    inline SwitchId switchId() const {return switch_id_;}
    inline TableId tableId() const {return table_id_;}
    inline RuleId id() const {return id_;}
    
    inline uint16_t priority() const {return priority_;}
    inline NetworkSpace& domain() const {return domain_;}
    inline std::vector<Action>& actions() const {return action_list_;}
    
    inline PortId& inPort() const {match_.in_port;}
    
    friend DependencyUpdater;
    
private:
    SwitchId switch_id_;
    TableId table_id_;
    RuleId id_;
    
    uint16_t priority_;
    NetworkSpace domain_;
    std::vector<Action> action_list_;
    
    std::vector<DependencyPtr> in_table_dependency_list_;
    std::vector<DependencyPtr> out_table_dependency_list_;
    
    std::vector<DependencyPtr> in_dependency_list_;
    std::vector<DependencyPtr> out_dependency_list_;

};

struct Dependency
{
    Dependency(RulePtr _src_rule, RulePtr _dst_rule,
               NetworkSpace _domain);
    
    RulePtr src_rule;
    RulePtr dst_rule;
    NetworkSpace domain;
    //TransferPtr transfer;
};

class RuleIterator
{
public:
    RuleIterator(SortedRuleMap::Iterator iterator,
                 SortedRuleMap& sorted_rule_map);
    
    bool operator!=(const RuleIterator& other);
    const RuleIterator& operator++();
    RulePtr& operator*() const;

private:
    SortedRuleMap::Iterator iterator_;
    RuleMap::iterator rule_map_iterator_;
    SortedRuleMap& sorted_rule_map_;
    
    void skip_empty_rule_maps();

};

class RuleRange
{
public:
    RuleRange(SortedRuleMap& sorted_rule_map);
    
    RuleIterator begin() const;
    RuleIterator end() const;

private:
    SortedRuleMap& sorted_rule_map_;

};

/*
using Graph = boost::adjacency_list<boost::listS,
                                    boost::listS,
                                    boost::bidirectionalS,
                                    RulePtr,
                                    DependencyPtr>;
using VertexDescriptor = Graph::vertex_descriptor;

struct DependencyGroup
{
    std::vector<DependencyPtr>::iterator begin()
    {
        return dependency_list.begin()
    }
    std::vector<DependencyPtr>::iterator end()
    {
        return dependency_list.end()
    }
    
    TransferPtr transfer;
    std::vector<DependencyPtr> dependency_list;
};


class DependencyIterator
{
public:
    using EdgeIterator = Graph::edge_iterator;
    DependencyIterator(EdgeIterator edge, Graph& graph):
        edge_(edge),
        graph_(graph) {}
    
    bool operator!=(const DependencyIterator& other)
    {
        // TODO: compare dependencies
        return (edge_ != other.edge_);
    }
    
    const DependencyIterator& operator++()
    {
        edge_++;
        return *this;
    }
    
    DependencyPtr& operator*() const
    {
        return graph[*edge_];
    }

private:
    EdgeIterator edge_;
    const Graph& graph_;
};

class DependencyRange
{
public:
    using EdgeIterator = Graph::edge_iterator;
    using Range = std::pair<EdgeIterator, EdgeIterator>;
    DependencyRange(Range range, Graph& graph):
        range_(range),
        graph_(graph) {}
    
    DependencyIterator begin() const
    {
        return DependencyIterator(graph, range_.first);
    }
    
    DependencyIterator end() const
    {
        return DependencyIterator(graph, range_.second);
    }

private:
    Range range_;
    const Graph& graph_;

};
*/
