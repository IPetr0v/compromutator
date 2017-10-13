#include "Rule.hpp"

Rule::Rule(SwitchId switch_id, TableId table_id, RuleId id,
           uint16_t priority, NetworkSpace& domain,
           std::vector<Action>& action_list):
    switch_id_(switch_id), table_id_(table_id), id_(id),
    priority_(priority), domain_(domain), action_list_(action_list)
{
    
}

// TODO: Delete this temporary solution
NetworkSpace Rule::outDomain() const
{
    return action_list_[1].transfer.apply(domain_);
}

Dependency::Dependency(RulePtr _src_rule, RulePtr _dst_rule,
                       NetworkSpace& _domain):
    src_rule(_src_rule),
    dst_rule(_dst_rule),
    domain(_domain)
{
    
}

RuleIterator::RuleIterator(SortedRuleMap::iterator iterator,
                           SortedRuleMap& sorted_rule_map):
    iterator_(iterator),
    sorted_rule_map_(sorted_rule_map)
{
    skip_empty_rule_maps();
}
    
bool RuleIterator::operator!=(const RuleIterator& other)
{
    RulePtr rule = rule_map_iterator_->second;
    RulePtr other_rule = other.rule_map_iterator_->second;
    return (rule->id() != other_rule->id());
}

const RuleIterator& RuleIterator::operator++()
{
    RuleMap& rule_map = iterator_->second;
    if (rule_map_iterator_ != rule_map.end())
        rule_map_iterator_++;
    else {
        iterator_++;
        skip_empty_rule_maps();
    }
    return *this;
}

RulePtr& RuleIterator::operator*() const
{
    return rule_map_iterator_->second;
}

void RuleIterator::skip_empty_rule_maps()
{
    while (iterator_ != sorted_rule_map_.end()) {
        RuleMap& rule_map = iterator_->second;
        if (rule_map.begin() == rule_map.end())
            iterator_++;
        else
            rule_map_iterator_ = rule_map.begin();
    }
}

RuleRange::RuleRange(SortedRuleMap& sorted_rule_map):
    sorted_rule_map_(sorted_rule_map),
    begin_(sorted_rule_map_.begin()),
    end_(sorted_rule_map_.end())
{

}


RuleRange::RuleRange(SortedRuleMap& sorted_rule_map,
                     SortedRuleMap::iterator begin,
                     SortedRuleMap::iterator end):
    sorted_rule_map_(sorted_rule_map),
    begin_(begin),
    end_(end)
{

}

RuleIterator RuleRange::begin() const
{
    return {begin_, sorted_rule_map_};
}

RuleIterator RuleRange::end() const
{
    return {end_, sorted_rule_map_};
}
