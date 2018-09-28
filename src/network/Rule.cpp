#include "Rule.hpp"

#include "Network.hpp"
#include "DependencyGraph.hpp"
#include "../flow_predictor/FlowPredictor.hpp"

#include <iostream>
#include <string>

std::shared_ptr<PortAction> Actions::getPortAction(PortId port_id) const
{
    auto it = std::find_if(port_actions.begin(), port_actions.end(),
        [port_id](const PortAction& port_action) -> bool {
           return port_action.port_type == PortType::NORMAL &&
                  port_action.port->id() == port_id;
        }
    );
    return port_actions.end() != it ? std::make_shared<PortAction>(*it)
                                    : nullptr;
}

IdGenerator<uint64_t> Rule::id_generator_;

Rule::Rule(RuleType type, SwitchPtr sw, TablePtr table, Priority priority,
           Cookie cookie, NetworkSpace&& domain, Actions&& actions):
    type_(type), table_(table), sw_(sw), priority_(priority), cookie_(cookie),
    match_(std::move(domain)), actions_(std::move(actions)),
    vertex_(VertexPtr(nullptr)),
    rule_mapping_(RuleMappingDescriptor(nullptr))
{
    TableId table_id = table_ ? table_->id() : (TableId)-1;
    SwitchId switch_id = sw_ ? sw_->id() : (SwitchId)-1;
    id_ = RuleId{switch_id, table_id, priority, id_generator_.getId()};
}

Rule::Rule(const RulePtr other, const NetworkSpace& domain):
    type_(other->type()), table_(other->table()), sw_(other->sw()),
    priority_(other->priority()), cookie_(other->cookie()), match_(domain),
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

ActionsBase Rule::actionsBase() const
{
    ActionsBase base;
    for (auto port_action : actions_.port_actions) {
        base.port_actions.push_back(
            static_cast<PortActionBase>(std::move(port_action)));
    }
    for (auto group_action : actions_.group_actions) {
        base.group_actions.push_back(
            static_cast<GroupActionBase>(std::move(group_action)));
    }
    for (auto table_action : actions_.table_actions) {
        base.table_actions.push_back(
            static_cast<TableActionBase>(std::move(table_action)));
    }
    return base;
}

bool Rule::isTableMiss() const
{
    return RuleType::FLOW == type_ && table_->tableMissRule()->id() == id_;
}

std::string Rule::toString() const
{
    std::string type;
    switch(type_) {
    case RuleType::FLOW:   type = "FLOW";   break;
    case RuleType::GROUP:  type = "GROUP";  break;
    case RuleType::BUCKET: type = "BUCKET"; break;
    case RuleType::SOURCE: type = "SOURCE"; break;
    case RuleType::SINK:   type = "SINK";   break;
    }
    auto sw = sw_ ? std::to_string(sw_->id()) : "NULL";
    auto table = table_ ? std::to_string(table_->id()) : "NULL";

    std::ostringstream os;
    os << "[" << type
       << ": sw=" << sw
       << ", table=" << table
       << ", prio=" << std::to_string(priority_)
       << ", cookie=" << std::hex << cookie_ << std::dec
       << ", domain=" << match_
       << "]";
    return os.str();
}

std::ostream& operator<<(std::ostream& os, const Rule& rule)
{
    os << rule.toString();
    return os;
}

std::ostream& operator<<(std::ostream& os, const RulePtr rule)
{
    os << rule->toString();
    return os;
}
