#pragma once

#include "ExampleNetwork.hpp"

#include "../src/network/DependencyGraph.hpp"

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
        auto link = network->addLink({1,2}, {2,1}).first;
        dependency_graph->addLink(link);

        table_miss1_diff = dependency_graph->addRule(table_miss1);
        table_miss2_diff = dependency_graph->addRule(table_miss2);

        rule1_diff = dependency_graph->addRule(rule1);
        rule2_diff = dependency_graph->addRule(rule2);
    }

    void destroyGraph() override {

        destroyNetwork();
    }

    EdgeDiff rule1_diff, rule2_diff;
    EdgeDiff table_miss1_diff, table_miss2_diff;
};
