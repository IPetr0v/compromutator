#include "Rule.hpp"

#include "Network.hpp"
#include "DependencyGraph.hpp"
#include "../flow_predictor/FlowPredictor.hpp"

#include <iostream>
#include <string>

IdGenerator<uint64_t> Rule::id_generator_;

Rule::Rule(RuleType type, SwitchPtr sw, TablePtr table, Priority priority,
           NetworkSpace&& domain, Actions&& actions):
    type_(type), table_(table), sw_(sw), priority_(priority),
    domain_(std::move(domain)), actions_(std::move(actions)),
    vertex_(VertexDescriptor(nullptr)),
    rule_mapping_(RuleMappingDescriptor(nullptr))
{
    TableId table_id = table_ ? table_->id() : (TableId)-1;
    SwitchId switch_id = sw_ ? sw_->id() : (SwitchId)-1;
    id_ = RuleId{switch_id, table_id, priority, id_generator_.getId()};
}

Rule::Rule(const RulePtr other, const NetworkSpace& domain):
    type_(other->type()), table_(other->table()), sw_(other->sw()),
    priority_(other->priority()), domain_(domain),
    actions_(other->actions())
{
    TableId table_id = table_ ? table_->id() : (TableId)-1;
    SwitchId switch_id = sw_ ? sw_->id() : (SwitchId)-1;
    id_ = RuleId{switch_id, table_id, priority_, id_generator_.getId()};
}

Rule::~Rule()
{
    auto rule_num = std::get<3>(id_);
    id_generator_.releaseId(rule_num);
}

std::ostream& operator<<(std::ostream& os, const Rule& rule)
{
    std::string type;
    switch(rule.type_) {
    case RuleType::FLOW:   type = "FLOW";   break;
    case RuleType::GROUP:  type = "GROUP";  break;
    case RuleType::BUCKET: type = "BUCKET"; break;
    case RuleType::SOURCE: type = "SOURCE"; break;
    case RuleType::SINK:   type = "SINK";   break;
    }
    auto sw = rule.sw_ ? std::to_string(rule.sw_->id()) : "NULL";
    auto table = rule.table_ ? std::to_string(rule.table_->id()) : "NULL";

    os << "[" << type
       << ": sw=" << sw
       << ", table=" << table
       << ", prio=" << std::to_string(rule.priority_)
       << ", domain=" << rule.domain_
       << "]";
    return os;
}

std::ostream& operator<<(std::ostream& os, const RulePtr rule)
{
    os << *rule;
    return os;
}
