#include "InterceptorManager.hpp"

InterceptorDiff& InterceptorDiff::operator+=(const InterceptorDiff& other)
{
    // Delete nonexistent rules
    auto it = std::remove_if(rules_to_add.begin(), rules_to_add.end(),
                             [other](RulePtr rule) {
                                 for (const auto other_rule : other.rules_to_delete) {
                                     if (other_rule->id() == rule->id()) {
                                         return true;
                                     }
                                 }
                                 return false;
                             }
    );
    rules_to_add.erase(it, rules_to_add.end());

    rules_to_add.insert(rules_to_add.end(),
                        other.rules_to_add.begin(),
                        other.rules_to_add.end());
    rules_to_delete.insert(rules_to_delete.end(),
                           other.rules_to_delete.begin(),
                           other.rules_to_delete.end());
    return *this;
}

std::vector<RuleInfo> InterceptorDiff::getRulesToAdd() const
{
    return getRules(rules_to_add);
}

std::vector<RuleInfo> InterceptorDiff::getRulesToDelete() const
{
    return getRules(rules_to_delete);
}

std::vector<RuleInfo>
InterceptorDiff::getRules(std::vector<RulePtr> original_rules) const
{
    std::vector<RuleInfo> rules;
    for (auto rule : original_rules) {
        auto in_port = rule->match().inPort();
        auto header = rule->match().header();
        for (auto& bit_space: header.getBitSpace()) {
            auto switch_id = rule->sw()->id();
            auto table_id = rule->table() ? rule->table()->id() : 0;
            auto cookie = rule->cookie();//0x1337;
            auto priority = rule->priority();
            auto match = Match(in_port, std::move(bit_space.mask));
            auto actions = ActionsBase::tableAction(1u);

            // Add main rule
            rules.emplace_back(
                switch_id, table_id, priority, cookie, match, actions);

            /*// TODO: add auxiliary difference rules
            // Add auxiliary rules
            for (auto& diff: bit_space.difference) {
                auto aux_priority = rule->priority();
                auto aux_match = Match(in_port, std::move(diff));
            }
            //rules.emplace_back()*/
        }
    }

    return rules;
}
