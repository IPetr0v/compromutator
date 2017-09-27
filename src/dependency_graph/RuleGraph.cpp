#include "RuleGraph.hpp"

typedef std::vector<PortId> PortList;
typedef std::vector<TableId> TableList;

struct NetworkState

struct State
{
    Header header;
    SwitchId switch_id;
    PortId port_id;
    TableId table_id;
};

struct StateAction

typedef std::vector<State> StateSpace;

class Transfer
{
public:
    StateSpace apply(const State& state) = 0;
    StateSpace inverse(const State& state) = 0;
    
    StateSpace domain() = 0;
    StateSpace range() = 0;

private:

};

void DependencyGraph::addVertex()
{
    // Create Rule
    Transfer transfer(match, action_list);
    RulePtr rule = make_shared(table_id, priority, transfer);
    TablePtr table = network_.getTable(switch_id, table_id);
    table.addRule(rule);
    
    // Add intratable dependency
    for (auto old_rule : rules_) {
        // Exclude same rule
        if (rule->id() == old_rule->id()) continue;
        
        // Check priority
        if (rule->priority() > old_rule->priority()) {
            // Check inport intersection
            if (!rule->match_inport() ||
                !old_rule->match_inport() ||
                rule->inport() == old_rule->inport())
            {
                
            }
        }
        else if (rule->priority() < old_rule->priority()) {
            
        }
        else if {
            return nullptr; // Undefinde behaviour
        }
    }
    
    // Add topology dependency
    
    // Add dependency graph node
    
    // Add rule dependency
    
    
    return rule_id;
}

void DependencyGraph::addLinkDependency()
{
    for (auto& src_rule : topology_.outRules(src_switch_id, src_port_id)) {
        auto& dst_rules = topology_.inRules(src_switch_id, src_port_id)
        rule_graph_.addArc(src_rule, dst_rules);
    }
}

void addRuleDependency(Rule& rule)
{
    // Evaluate dependency
    for (auto& lower_rule : rule.table().rules()) {
        addEdges(rule, table);
    }
    
    // Add in arcs
    for (auto& prev_table : network_.inputTables(rule)) {
        for (auto& prev_rule : prev_table.rules) {
            
        }
    }
    
    
    // Add out arcs
    for (auto& table : rule.outputTables()) {
        addArcs(rule, table);
    }
}

void addArcs(Rule& src_rule, Table& dst_table)
{
    Domain output_domain = rule.outputDomain();
    for (auto& dst_rule : dst_table.rulesFromPort()) {
        Domain arc_domain = next_rule.domain() & output_domain;
        output_domain -= arc_domain;
        addArc(src_rule, dst_rule, arc_domain);
    }
}

NodePtr DependencyGraph::DeleteNode()
{
    
}


HeaderSpace transfer(HeaderSpace& header)
{
    return transfer_.apply(header);
}

HeaderSpace transferInverse(HeaderSpace& header)
{
    return transfer_.inverse(header);
}
