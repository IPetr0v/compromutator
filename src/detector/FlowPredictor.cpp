#include "FlowPredictor.hpp"

FlowPredictor::FlowPredictor(DependencyGraph& dependency_graph):
    dependency_graph_(dependency_graph)
{
    
}

void FlowPredictor::addRule(RulePtr rule)
{
    
}

void FlowPredictor::deleteRule(RulePtr rule)
{
    
}

uint64_t FlowPredictor::getCounter(SwitchId switch_id,
                                   TableId table_id,
                                   RuleId rule_id)
{
    return 0;
}
