#include "Rule.hpp"

#include "Network.hpp"
#include "DependencyGraph.hpp"
#include "../flow_predictor/FlowPredictor.hpp"

#include <tuple>

IdGenerator<uint64_t> Rule::id_generator_;

Rule::Rule(RuleType type, SwitchPtr sw, TablePtr table, Priority priority,
           NetworkSpace&& domain, Actions&& actions):
    type_(type), table_(table), sw_(sw), priority_(priority),
    domain_(std::move(domain)), actions_(std::move(actions)),
    vertex_(VertexDescriptor(nullptr)),
    rule_mapping_(RuleMappingDescriptor(nullptr))
{
    assert(nullptr != sw);
    // If the rule is a flow then the getTable and getSwitch pointers
    // should not be null
    assert(type != RuleType::FLOW && (table != nullptr && sw != nullptr));
    TableId table_id = table_ ? table_->id() : (TableId)-1;
    SwitchId switch_id = sw_ ? sw_->id() : (SwitchId)-1;
    id_ = RuleId{switch_id, table_id, priority, id_generator_.getId()};
}

Rule::Rule(const RulePtr other, NetworkSpace&& domain):
    type_(other->type()), table_(other->table()), sw_(other->sw()),
    priority_(other->priority()), domain_(std::move(domain)),
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
