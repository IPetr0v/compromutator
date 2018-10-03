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
           Cookie cookie, Match&& match, Actions&& actions):
    type_(type), table_(table), sw_(sw), priority_(priority), cookie_(cookie),
    match_(match), domain_(NetworkSpace(match)),
    actions_(std::move(actions)), vertex_(VertexPtr(nullptr)),
    rule_mapping_(RuleMappingDescriptor(nullptr))
{
    TableId table_id = table_ ? table_->id() : (TableId)-1;
    SwitchId switch_id = sw_ ? sw_->id() : (SwitchId)-1;
    id_ = RuleId{switch_id, table_id, priority, id_generator_.getId()};
}

Rule::Rule(RuleType type, SwitchPtr sw, TablePtr table, Priority priority,
           Cookie cookie, NetworkSpace&& domain, Actions&& actions):
    type_(type), table_(table), sw_(sw), priority_(priority), cookie_(cookie),
    match_(domain.match()), domain_(std::move(domain)),
    actions_(std::move(actions)), vertex_(VertexPtr(nullptr)),
    rule_mapping_(RuleMappingDescriptor(nullptr))
{
    TableId table_id = table_ ? table_->id() : (TableId)-1;
    SwitchId switch_id = sw_ ? sw_->id() : (SwitchId)-1;
    id_ = RuleId{switch_id, table_id, priority, id_generator_.getId()};
}

Rule::Rule(const RulePtr other, Cookie cookie, const NetworkSpace& domain):
    type_(other->type()), table_(other->table()), sw_(other->sw()),
    priority_(other->priority()), cookie_(cookie), match_(domain.match()),
    domain_(domain), actions_(other->actions())
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

RuleInfoPtr Rule::info() const
{
    auto switch_id = sw()->id();
    auto table_id = table() ? table()->id() : (TableId) 0;
    auto match = match_;
    auto actions = actionsBase();
    return std::make_shared<RuleInfo>(
        switch_id, table_id, priority_, cookie_,
        std::move(match), std::move(actions), id_);
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

size_t Rule::hash() const
{
    return std::hash<uint64_t>{}(std::get<3>(id_));
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
       << ", domain=" << domain_
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

RuleInfo::RuleInfo(SwitchId switch_id, TableId table_id, Priority priority,
                   Cookie cookie, Match match, ActionsBase actions,
                   RuleId rule_id):
    switch_id(switch_id), table_id(table_id), priority(priority),
    cookie(cookie), match(std::move(match)), actions(std::move(actions)),
    rule_id_(rule_id)
{

}

bool RuleInfo::operator==(const RuleInfo& other) const
{
    if (not std::get<3>(rule_id_)) {
        return false;
    }
    else {
        return rule_id_ == other.rule_id_;
    }
}

std::ostream& operator<<(std::ostream& os, const RuleInfo& rule)
{
    auto domain = rule.match;
    os << "[" << "INFO"
       << ": sw=" << rule.switch_id
       << ", table=" << std::to_string(rule.table_id)
       << ", prio=" << std::to_string(rule.priority)
       << ", cookie=" << std::hex << rule.cookie << std::dec
       << ", domain=" << NetworkSpace(std::move(domain))
       << "]";
    return os;
}
