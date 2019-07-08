#include "gtest/gtest.h"

#include "ExampleNetwork.hpp"
#include "../../src/NetworkSpace.hpp"
#include "../../src/network/DependencyGraph.hpp"

#include <algorithm>
#include <memory>
#include <set>

class GraphTest : public ::testing::Test
{
    struct Data {
        explicit Data(int data = 0): data(data) {}
        int data;
    };
protected:
    using GraphData = Graph<Data, Data>;
    using Vertex = GraphData::VertexPtr;
    using Edge = GraphData::EdgePtr;

    virtual void SetUp() {
        graph = std::make_shared<GraphData>();
        v1 = graph->addVertex(1);
        v2 = graph->addVertex(2);
        v3 = graph->addVertex(3);
        v4 = graph->addVertex(4);
    }

    virtual void TearDown() {
        graph.reset();
    }

    std::shared_ptr<GraphData> graph;
    Vertex v1, v2, v3, v4;
};

TEST_F(GraphTest, DataTest)
{
    EXPECT_EQ(1, v1->data);
    EXPECT_EQ(2, v2->data);

    graph->addEdge(v1, v2, 3);
    auto edge_pair = graph->edge(v1, v2);
    bool edge_found = edge_pair.second;
    ASSERT_EQ(true, edge_found);
    auto edge = edge_pair.first;
    EXPECT_EQ(1, edge->src->data);
    EXPECT_EQ(2, edge->dst->data);
    EXPECT_EQ(3, edge->data);
}

TEST_F(GraphTest, DeleteEdgesTest)
{
    graph->addEdge(v1, v2);
    graph->deleteEdge(v1, v2);
    EXPECT_TRUE(graph->edges().empty());

    graph->addEdge(v1, v4);
    graph->addEdge(v2, v4);
    graph->addEdge(v3, v4);
    EXPECT_EQ(3u, graph->edges().size());

    graph->deleteOutEdges(v1);
    EXPECT_EQ(0u, graph->outDegree(v1));
    EXPECT_EQ(2u, graph->inDegree(v4));

    graph->deleteInEdges(v4);
    EXPECT_EQ(0u, graph->inDegree(v2));
    EXPECT_EQ(0u, graph->inDegree(v3));
    EXPECT_EQ(0u, graph->inDegree(v4));
    EXPECT_TRUE(graph->edges().empty());
}

class InitDependencyGraphTest : public ::testing::Test
                              , public TwoSwitchNetwork
{
protected:
    virtual void SetUp() {
        initNetwork();
        dependency_graph = std::make_shared<DependencyGraph>(network);
    }

    virtual void TearDown() {
        destroyNetwork();
        dependency_graph.reset();
    }

    std::shared_ptr<DependencyGraph> dependency_graph;
};

TEST_F(InitDependencyGraphTest, CreationTest)
{
    network->deleteRule(rule1->id());
    network->deleteRule(rule2->id());
    network->deleteRule(rule3->id());
    network->deleteRule(rule4->id());
    network->deleteRule(rule5->id());
    network->deleteRule(rule11->id());

    // Add special rules
    dependency_graph->addRule(network->dropRule());
    EXPECT_TRUE(dependency_graph->popEdgeDiff().empty());
    dependency_graph->addRule(network->controllerRule());
    EXPECT_TRUE(dependency_graph->popEdgeDiff().empty());
    for (auto port : {port11, port12, port21, port22}) {
        dependency_graph->addRule(port->sourceRule());
        EXPECT_TRUE(dependency_graph->popEdgeDiff().empty());
        dependency_graph->addRule(port->sinkRule());
        EXPECT_TRUE(dependency_graph->popEdgeDiff().empty());
    }

    // Add table miss rules
    dependency_graph->addRule(table_miss);
    auto diff = dependency_graph->popEdgeDiff();
    ASSERT_EQ(1u, diff.new_edges.size());
    auto new_edge = diff.new_edges.begin();
    EXPECT_EQ(table_miss->id(), new_edge->src->id());
    EXPECT_EQ(network->dropRule(), new_edge->dst);
    ASSERT_EQ(2u, diff.new_dependent_edges.size());
    std::set<std::pair<RulePtr, RulePtr>> new_dependent_edges {
        {port11->sourceRule(), table_miss},
        {port12->sourceRule(), table_miss}
    };
    for (const auto& edge : diff.new_dependent_edges) {
        auto it = new_dependent_edges.find({edge.src, edge.dst});
        EXPECT_NE(new_dependent_edges.end(), it);
        new_dependent_edges.erase(it);
    }
    EXPECT_TRUE(new_dependent_edges.empty());
    dependency_graph->addRule(table_miss_sw2);
    dependency_graph->popEdgeDiff();

    // Add link
    dependency_graph->addLink(link);
    auto link_diff = dependency_graph->popEdgeDiff();
    EXPECT_TRUE(link_diff.new_edges.empty());
    EXPECT_TRUE(link_diff.new_dependent_edges.empty());
    EXPECT_TRUE(link_diff.changed_edges.empty());
    ASSERT_EQ(2u, link_diff.removed_edges.size());
    std::list<Dependency> expected_edges {
        {port12->sourceRule(), table_miss},
        {port21->sourceRule(), table_miss_sw2}
    };
    for (auto edge : link_diff.removed_edges) {
        auto it = std::find(expected_edges.begin(),
                            expected_edges.end(),
                            edge);
        EXPECT_NE(expected_edges.end(), it);
        expected_edges.erase(it);
    }
    EXPECT_TRUE(expected_edges.empty());
}

class DependencyGraphTest : public ::testing::Test
                          , public SimpleTwoSwitchNetwork
{
protected:
    virtual void SetUp() {
        initNetwork();
        dependency_graph = std::make_shared<DependencyGraph>(network);
        dependency_graph->addRule(network->dropRule());
        dependency_graph->addRule(network->controllerRule());
        for (auto port : {port11, port12, port21, port22}) {
            dependency_graph->addRule(port->sourceRule());
            dependency_graph->addRule(port->sinkRule());
        }
        dependency_graph->addRule(table_miss1);
        dependency_graph->addRule(table_miss2);
        dependency_graph->popEdgeDiff();
    }

    virtual void TearDown() {
        destroyNetwork();
        dependency_graph.reset();
    }

    //using Desc = EdgePtr;
    struct EdgeInfo {RulePtr src_rule, dst_rule; N domain;};
    EdgeInfo getEdgeInfo(Dependency dependency) const {
        auto src_rule = dependency.src;
        auto dst_rule = dependency.dst;
        auto domain = dependency.domain;
        return EdgeInfo{src_rule, dst_rule, domain};
    }
    EdgeInfo findEdgeFrom(const std::list<Dependency>& edges,
                          RulePtr rule) const {
        auto it = std::find_if(edges.begin(), edges.end(),
            [rule](Dependency dependency) -> bool {
                return rule == dependency.src;
            }
        );
        return it != edges.end() ? getEdgeInfo(*it)
                                 : EdgeInfo{nullptr, nullptr, N::emptySpace()};
    }
    EdgeInfo findEdgeTo(const std::list<Dependency>& edges,
                        RulePtr rule) const {
        auto it = std::find_if(edges.begin(), edges.end(),
            [rule](Dependency dependency) -> bool {
               return rule == dependency.dst;
            }
        );
        return it != edges.end() ? getEdgeInfo(*it)
                                 : EdgeInfo{nullptr, nullptr, N::emptySpace()};
    }

    std::shared_ptr<DependencyGraph> dependency_graph;
};

TEST_F(DependencyGraphTest, AddRuleTest)
{
    dependency_graph->addRule(rule1);
    auto rule_diff = dependency_graph->popEdgeDiff();
    ASSERT_EQ(1u, rule_diff.new_edges.size());
    ASSERT_EQ(1u, rule_diff.new_dependent_edges.size());
    ASSERT_EQ(1u, rule_diff.changed_edges.size());
    EXPECT_TRUE(rule_diff.removed_edges.empty());

    auto in_edge = findEdgeFrom(
        rule_diff.new_dependent_edges, port11->sourceRule()
    );
    EXPECT_EQ(port11->sourceRule(), in_edge.src_rule);
    EXPECT_EQ(rule1, in_edge.dst_rule);
    EXPECT_EQ(N(1, H("0000xxxx")), in_edge.domain);

    auto out_edge = findEdgeFrom(rule_diff.new_edges, rule1);
    EXPECT_EQ(rule1, out_edge.src_rule);
    EXPECT_EQ(port12->sinkRule(), out_edge.dst_rule);
    EXPECT_EQ(N(2, H("0000xxxx")), out_edge.domain);

    auto changed_edge = *rule_diff.changed_edges.begin();
    EXPECT_EQ(port11->sourceRule(), changed_edge.src);
    EXPECT_EQ(table_miss1, changed_edge.dst);
    EXPECT_EQ(N(port11->id()) - N(1, H("0000xxxx")), changed_edge.domain);

    auto top_rule = network->addRule(1, 0, 2, 0x0, M(1, B("00000011")),
                                     ActionsBase::portAction(2));
    dependency_graph->addRule(top_rule);
    auto top_diff = dependency_graph->popEdgeDiff();
    ASSERT_EQ(1u, top_diff.changed_edges.size());
    auto last_edge = findEdgeTo(top_diff.changed_edges, rule1);
    EXPECT_EQ(port11->sourceRule(), last_edge.src_rule);
    EXPECT_EQ(rule1, last_edge.dst_rule);
    EXPECT_EQ(N(1, H("0000xxxx")) - N(1, H("00000011")), last_edge.domain);
}

TEST_F(DependencyGraphTest, DeleteRuleTest)
{
    dependency_graph->addRule(rule1);
    dependency_graph->popEdgeDiff();
    dependency_graph->deleteRule(rule1);
    auto rule_diff = dependency_graph->popEdgeDiff();
    EXPECT_TRUE(rule_diff.new_edges.empty());
    EXPECT_TRUE(rule_diff.new_dependent_edges.empty());
    ASSERT_EQ(1u, rule_diff.changed_edges.size());
    ASSERT_EQ(1u, rule_diff.removed_edges.size());
    ASSERT_EQ(1u, rule_diff.removed_dependent_edges.size());

    auto& changed_table_miss_edge = *rule_diff.changed_edges.begin();
    EXPECT_EQ(port11->sourceRule(), changed_table_miss_edge.src);
    EXPECT_EQ(table_miss1, changed_table_miss_edge.dst);
    EXPECT_EQ(N(1, H("xxxxxxxx")), changed_table_miss_edge.domain);

    auto deleted_out_edge = *rule_diff.removed_edges.begin();
    EXPECT_EQ(rule1, deleted_out_edge.src);
    EXPECT_EQ(port12->sinkRule(), deleted_out_edge.dst);

    auto deleted_in_edge = *rule_diff.removed_dependent_edges.begin();
    EXPECT_EQ(port11->sourceRule(), deleted_in_edge.src);
    EXPECT_EQ(rule1, deleted_in_edge.dst);

}

TEST_F(DependencyGraphTest, AddLinkTest)
{
    dependency_graph->addRule(rule1);
    dependency_graph->addRule(rule2);
    dependency_graph->popEdgeDiff();

    auto link = network->addLink({1,2}, {2,1}).first;
    dependency_graph->addLink(link);
    auto diff = dependency_graph->popEdgeDiff();
    ASSERT_EQ(2u, diff.new_edges.size());
    EXPECT_EQ(0u, diff.changed_edges.size());
    EXPECT_EQ(4u, diff.removed_edges.size());

    auto link_edge_to2 = findEdgeTo(diff.new_edges, rule2);
    EXPECT_EQ(rule1, link_edge_to2.src_rule);
    EXPECT_EQ(rule2, link_edge_to2.dst_rule);
    EXPECT_EQ(N(1, H("000000xx")), link_edge_to2.domain);

    auto link_edge_to_miss = findEdgeTo(diff.new_edges, table_miss2);
    EXPECT_EQ(rule1, link_edge_to_miss.src_rule);
    EXPECT_EQ(table_miss2, link_edge_to_miss.dst_rule);
    EXPECT_EQ(N(1, H("0000xxxx")) - N(1, H("000000xx")),
              link_edge_to_miss.domain);

    auto port12_source_range = dependency_graph->outEdges(port12->sourceRule());
    auto port12_sink_range = dependency_graph->inEdges(port12->sinkRule());
    auto port21_source_range = dependency_graph->outEdges(port21->sourceRule());
    auto port21_sink_range = dependency_graph->inEdges(port21->sinkRule());
    EXPECT_TRUE(port12_source_range.empty());
    EXPECT_TRUE(port12_sink_range.empty());
    EXPECT_TRUE(port21_source_range.empty());
    EXPECT_TRUE(port21_sink_range.empty());
}

TEST_F(DependencyGraphTest, ExistingLinkTest)
{
    dependency_graph->addRule(rule2);
    network->deleteRule(rule1->id());
    auto link = network->addLink({1,2}, {2,1}).first;
    dependency_graph->addLink(link);
    dependency_graph->popEdgeDiff();

    auto rule = network->addRule(1, 0, 1, 0x0, M(1, B("0000xxxx")),
                                 ActionsBase::portAction(2));
    dependency_graph->addRule(rule);
    auto diff = dependency_graph->popEdgeDiff();

    auto link_edge_to2 = findEdgeTo(diff.new_edges, rule2);
    EXPECT_EQ(rule, link_edge_to2.src_rule);
    EXPECT_EQ(rule2, link_edge_to2.dst_rule);
    EXPECT_EQ(N(1, H("000000xx")), link_edge_to2.domain);

    auto link_edge_to_miss = findEdgeTo(diff.new_edges, table_miss2);
    EXPECT_EQ(rule, link_edge_to_miss.src_rule);
    EXPECT_EQ(table_miss2, link_edge_to_miss.dst_rule);
    EXPECT_EQ(N(1, H("0000xxxx")) - N(1, H("000000xx")),
              link_edge_to_miss.domain);
}

TEST_F(DependencyGraphTest, DeleteLinkTest)
{
    dependency_graph->addRule(rule1);
    dependency_graph->addRule(rule2);
    auto new_link = network->addLink({1,2}, {2,1}).first;
    dependency_graph->addLink(new_link);
    dependency_graph->popEdgeDiff();

    auto deleted_link = network->deleteLink({1,2}, {2,1}).first;
    dependency_graph->deleteLink(deleted_link);
    auto diff = dependency_graph->popEdgeDiff();
    EXPECT_EQ(4u, diff.new_edges.size());
    EXPECT_EQ(0u, diff.changed_edges.size());
    EXPECT_EQ(2u, diff.removed_edges.size());

    auto edge_from_rule1 = findEdgeFrom(diff.new_edges, rule1);
    EXPECT_EQ(rule1, edge_from_rule1.src_rule);
    EXPECT_EQ(port12->sinkRule(), edge_from_rule1.dst_rule);
    EXPECT_EQ(N(2, H("0000xxxx")), edge_from_rule1.domain);

    auto edge_to_table_miss1 = findEdgeTo(diff.new_edges, table_miss1);
    EXPECT_EQ(port12->sourceRule(), edge_to_table_miss1.src_rule);
    EXPECT_EQ(table_miss1, edge_to_table_miss1.dst_rule);
    EXPECT_EQ(N(2), edge_to_table_miss1.domain);

    auto edge_to_rule2 = findEdgeTo(diff.new_edges, rule2);
    EXPECT_EQ(port21->sourceRule(), edge_to_rule2.src_rule);
    EXPECT_EQ(rule2, edge_to_rule2.dst_rule);
    EXPECT_EQ(N(1, H("000000xx")), edge_to_rule2.domain);

    auto edge_to_table_miss2 = findEdgeTo(diff.new_edges, table_miss2);
    EXPECT_EQ(port21->sourceRule(), edge_to_table_miss2.src_rule);
    EXPECT_EQ(table_miss2, edge_to_table_miss2.dst_rule);
    EXPECT_EQ(N(1, H("xxxxxxxx") - H("000000xx")), edge_to_table_miss2.domain);

    std::set<std::pair<RulePtr, RulePtr>> expected_removed_edges {
        {rule1, rule2}, {rule1, table_miss2}
    };
    for (auto edge : diff.removed_edges) {
        auto src_rule = edge.src;
        auto dst_rule = edge.dst;
        auto it = expected_removed_edges.find({src_rule, dst_rule});
        EXPECT_NE(expected_removed_edges.end(), it);
        expected_removed_edges.erase(it);
    }
    EXPECT_TRUE(expected_removed_edges.empty());
}

TEST_F(DependencyGraphTest, EdgeCleanUpTest)
{
    network->deleteRule(rule1->id());
    network->deleteRule(rule2->id());
    auto rule = network->addRule(1, 0, 1, 0x0, M(1, B("xxxxxxxx")),
                                 ActionsBase::portAction(2));
    dependency_graph->addRule(rule);
    auto diff = dependency_graph->popEdgeDiff();
    ASSERT_EQ(1u, diff.removed_edges.size());
    EXPECT_TRUE(diff.changed_edges.empty());
    auto edge = *diff.removed_edges.begin();
    EXPECT_EQ(port11->sourceRule(), edge.src);
    EXPECT_EQ(table_miss1, edge.dst);
    EXPECT_NE(rule, edge.dst);
}

TEST_F(DependencyGraphTest, TableRuleTest)
{
    dependency_graph->addRule(rule1);
    dependency_graph->addRule(rule2);
    dependency_graph->addRule(
        network->getSwitch(1)->addTable(1)->tableMissRule()
    );

    auto table1_rule = network->addRule(1, 1, 1, 0x0, M(1, B("000000xx")),
                                        ActionsBase::portAction(2));
    dependency_graph->addRule(table1_rule);
    dependency_graph->popEdgeDiff();
    // TODO: check this test, may be wrong
    auto rule = network->addRule(1, 0, 2, 0x0, M(1, B("00000011")),
                                 ActionsBase::tableAction(1));
    dependency_graph->addRule(rule);
    auto diff = dependency_graph->popEdgeDiff();
    ASSERT_EQ(1u, diff.new_edges.size());
    ASSERT_EQ(1u, diff.new_dependent_edges.size());
    ASSERT_EQ(1u, diff.changed_edges.size());
    EXPECT_TRUE(diff.removed_edges.empty());
    auto in_edge = findEdgeTo(
        diff.changed_edges, rule1
    );
    EXPECT_EQ(port11->sourceRule(), in_edge.src_rule);
    EXPECT_EQ(rule1, in_edge.dst_rule);
    EXPECT_EQ(N(1, H("0000xxxx")) - N(1, H("00000011")), in_edge.domain);
    auto out_edge = findEdgeFrom(
        diff.new_edges, rule
    );
    EXPECT_EQ(rule, out_edge.src_rule);
    EXPECT_EQ(table1_rule, out_edge.dst_rule);
    EXPECT_EQ(N(1, H("00000011")), out_edge.domain);
}
