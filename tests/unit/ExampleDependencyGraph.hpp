#pragma once

#include "ExampleNetwork.hpp"

#include "../../src/network/DependencyGraph.hpp"

#include <memory>

class ExampleDependencyGraph
{
protected:
    using H = HeaderSpace;
    using N = NetworkSpace;
    using T = Transfer;

    virtual void initGraph() = 0;
    virtual void destroyGraph() = 0;

    std::shared_ptr<DependencyGraph> dependency_graph;
};

class SimpleTwoSwitchGraph : public ExampleDependencyGraph
                           , public SimpleTwoSwitchNetwork
{
protected:
    void initGraph() override {
        initNetwork();
        dependency_graph = std::make_shared<DependencyGraph>(network);
    }

    void destroyGraph() override {
        dependency_graph.reset();
        destroyNetwork();
    }

    void installSpecialRules() {
        dependency_graph->addRule(network->dropRule());
        drop_diff = dependency_graph->popEdgeDiff();
        dependency_graph->addRule(network->controllerRule());
        controller_diff = dependency_graph->popEdgeDiff();

        dependency_graph->addRule(port11->sourceRule());
        p11_source_diff = dependency_graph->popEdgeDiff();
        dependency_graph->addRule(port11->sinkRule());
        p11_sink_diff = dependency_graph->popEdgeDiff();
        dependency_graph->addRule(port12->sourceRule());
        p12_source_diff = dependency_graph->popEdgeDiff();
        dependency_graph->addRule(port12->sinkRule());
        p12_sink_diff = dependency_graph->popEdgeDiff();

        dependency_graph->addRule(port21->sourceRule());
        p21_source_diff = dependency_graph->popEdgeDiff();
        dependency_graph->addRule(port21->sinkRule());
        p21_sink_diff = dependency_graph->popEdgeDiff();
        dependency_graph->addRule(port22->sourceRule());
        p22_source_diff = dependency_graph->popEdgeDiff();
        dependency_graph->addRule(port22->sinkRule());
        p22_sink_diff = dependency_graph->popEdgeDiff();
    }
    void installTableMissRules() {
        dependency_graph->addRule(table_miss1);
        table_miss1_diff = dependency_graph->popEdgeDiff();
        dependency_graph->addRule(table_miss2);
        table_miss2_diff = dependency_graph->popEdgeDiff();
    }
    void installLink() {
        auto link = network->addLink({1,2}, {2,1}).first;
        dependency_graph->addLink(link);
        link_diff = dependency_graph->popEdgeDiff();
    }
    void deleteLink() {
        auto link = network->link({1,2}, {2,1}).first;
        dependency_graph->deleteLink(link);
        link_deletion_diff = dependency_graph->popEdgeDiff();
    }
    void installFirstRule() {
        dependency_graph->addRule(rule1);
        rule1_diff = dependency_graph->popEdgeDiff();
    }
    void installSecondRule() {
        dependency_graph->addRule(rule2);
        rule2_diff = dependency_graph->popEdgeDiff();
    }

    EdgeDiff drop_diff, controller_diff;
    EdgeDiff p11_source_diff, p11_sink_diff;
    EdgeDiff p12_source_diff, p12_sink_diff;
    EdgeDiff p21_source_diff, p21_sink_diff;
    EdgeDiff p22_source_diff, p22_sink_diff;
    EdgeDiff table_miss1_diff, table_miss2_diff;
    EdgeDiff link_diff, link_deletion_diff;
    EdgeDiff rule1_diff, rule2_diff;
};
