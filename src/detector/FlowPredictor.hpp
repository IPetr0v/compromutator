#pragma once

#include "../Common.hpp"
#include "../dependency_graph/DependencyGraph.hpp"

class FlowPredictor
{
public:
    FlowPredictor(DependencyGraph& dependency_graph);
    
    void addRule(RulePtr rule);
    void deleteRule(RulePtr rule);
    
    uint64_t getCounter(SwitchId switch_id,
                        TableId table_id,
                        RuleId rule_id);

private:
    DependencyGraph& dependency_graph_;

};
