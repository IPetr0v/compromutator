#pragma once

#include "../Common.hpp"
#include "../NetworkSpace.hpp"

#include <list>
#include <map>
#include <memory>
#include <vector>

class Rule;
class Dependency;

class RuleIdGenerator;
class Action;

typedef std::shared_ptr<Rule> RulePtr;
typedef std::shared_ptr<Dependency> DependencyPtr;

typedef std::map<RuleId, RulePtr> RuleMap;
typedef std::map<RuleId, Priority> PriorityMap;
typedef std::map<Priority, RuleMap, std::greater<Priority>> SortedRuleMap;

enum class RuleType
{
    TABLE,
    GROUP,
    SOURCE,
    SINK,
    CONTROLLER,
    DROP
};

class Rule
{
public:
    Rule(SwitchId switch_id, TableId table_id, uint16_t priority,
         NetworkSpace& domain, std::vector<Action>& action_list);
    ~Rule();

    RuleType type() const {return type_;}
    
    SwitchId switchId() const {return switch_id_;}
    PortId portId() const {return port_id_;}
    TableId tableId() const {return table_id_;}
    GroupId groupId() const {return group_id_;}
    RuleId id() const {return id_;}
    
    uint16_t priority() const {return priority_;}
    NetworkSpace domain() const {return domain_;}
    std::vector<Action> actions() const {return action_list_;}
    
    PortId inPort() const {domain_.inPort();}
    NetworkSpace outDomain() const;
    
    friend class DependencyUpdater;
    
private:
    RuleType type_;

    SwitchId switch_id_;
    PortId port_id_;
    TableId table_id_;
    GroupId group_id_;
    RuleId id_;
    
    uint16_t priority_;
    NetworkSpace domain_;
    std::vector<Action> action_list_;
    
    std::list<DependencyPtr> in_table_dependency_list_;
    std::list<DependencyPtr> out_table_dependency_list_;
    
    std::list<DependencyPtr> in_dependency_list_;
    std::list<DependencyPtr> out_dependency_list_;

};

struct Dependency
{
    Dependency(RulePtr _src_rule, RulePtr _dst_rule,
               NetworkSpace& _domain);
    
    RulePtr src_rule;
    RulePtr dst_rule;
    NetworkSpace domain;
    //TransferPtr transfer;
};

// Singleton for rule id generation
class RuleIdGenerator
{
public:
    static RuleId getId();
    static void releaseId(RuleId id);

    RuleIdGenerator(RuleIdGenerator const&) = delete;
    RuleIdGenerator& operator= (RuleIdGenerator const&) = delete;

private:
    RuleIdGenerator();
    ~RuleIdGenerator() = default;

    RuleId last_id_;

};

class RuleIterator
{
public:
    RuleIterator(SortedRuleMap::iterator iterator,
                 SortedRuleMap& sorted_rule_map);
    
    bool operator!=(const RuleIterator& other);
    const RuleIterator& operator++();
    RulePtr& operator*() const;

private:
    SortedRuleMap::iterator iterator_;
    RuleMap::iterator rule_map_iterator_;
    SortedRuleMap& sorted_rule_map_;
    
    void set_rule_map_iterator();

};

class RuleRange
{
public:
    explicit RuleRange(SortedRuleMap& sorted_rule_map);
    // TODO: make partial maps and vectors in RuleRange better than this!
    RuleRange(SortedRuleMap& sorted_rule_map,
              SortedRuleMap::iterator begin,
              SortedRuleMap::iterator end);
    
    RuleIterator begin() const;
    RuleIterator end() const;

private:
    SortedRuleMap& sorted_rule_map_;
    SortedRuleMap::iterator begin_;
    SortedRuleMap::iterator end_;

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
