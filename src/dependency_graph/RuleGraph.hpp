#pragma once

using namespace boost;

class DependencyGraph
{
public:
    DependencyGraph(int header_length):
        header_length_(header_length) {}
    
    SwitchId addSwitch(SwitchId id, std::vector<PortNum>& port_list);
    void deleteSwtich(SwitchId id);
    
    TableId addTable(SwitchId switch_id, TableId table_id);
    void deleteTable(SwitchId switch_id, TableId table_id);
    
    RuleId addRule(SwitchId switch_id, TableId table_id,
                   uint16_t priority, Match& match,
                   std::vector<Action>& action_list);
    void deleteRule(SwitchId switch_id, TableId table_id,
                    RuleId rule_id);
    
    void addLink(SwitchId src_switch_id, PortId src_port_id,
                 SwitchId dst_switch_id, PortId dst_port_id);
    void deleteLink(SwitchId src_switch_id, PortId src_port_id,
                    SwitchId dst_switch_id, PortId dst_port_id);

private:
    int header_length_;
    
    Network network_;
    RuleTopology rule_topology_;
    
    EdgePtr AddEdge();
    EdgePtr DeleteEdge();

};

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/labeled_graph.hpp>
#include <boost/property_map/property_map.hpp>

//typedef adjacency_list<listS, listS, undirectedS, int> Graph;
//typedef labeled_graph<Graph, std::string> LabeledGraph;
//typedef Graph::vertex_descriptor vertex_descriptor;

enum class NodeType
{
    RULE, SOURCE, TARGET, CONTROLLER, DROP
};

struct Vertex
{
    NodeTyle type;
    RulePtr rule;
    
    PartitionId partitionId() {return PartitionId{rule->switchId(),
                                                  rule->tableId()};}
    
    domain;
    transfer;
};

struct Arc
{
    HeaderSpace domain;
};

class RuleGraph
{
public:
    RuleGraph();
    ~RuleGraph();
    
    addVertex(RulePtr rule);
    addArcs(RulePtr src_rule,
            std::vector<RulePtr>& dst_rules);

private:
    struct PartitionId{SwitchId switch_id; TableId table_id};
    using Graph = boost::adjacency_list<boost::listS,
                                        boost::listS,
                                        boost::bidirectionalS,
                                        Vertex,
                                        Arc>;
    using vertex_descriptor = Graph::vertex_descriptor;
    
    std::map<RuleId, vertex_descriptor> rule_map_;
    std::map<PartitionId, std::list<vertex_descriptor>> partition_map_;
    
};

class RuleTopology
{
public:
    RuleTopology(int header_length):
        header_length_(header_length) {}
    
    void addLink(SwitchPtr src_switch, PortId src_port_id,
                 SwitchPtr dst_switch, PortId dst_port_id);
    
    addVertex(RulePtr rule);
    addArcs(RulePtr src_rule, std::vector<RulePtr>& dst_rules);

private:
    int header_length_;
    
    Topology topology_;
    
    EdgePtr AddEdge();
    EdgePtr DeleteEdge();

};
