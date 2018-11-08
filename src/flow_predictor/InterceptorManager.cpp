#include "InterceptorManager.hpp"

InterceptorDiff& InterceptorDiff::operator+=(InterceptorDiff&& other)
{
    // Delete nonexistent rules
    auto it = std::remove_if(rules_to_add.begin(), rules_to_add.end(),
        [&other](RuleInfoPtr rule) {
        for (auto other_it = other.rules_to_delete.begin();
             other_it != other.rules_to_delete.end();
             other_it++) {
            auto other_rule = *other_it;
            if (*other_rule == *rule) {
                other.rules_to_delete.erase(other_it);
                return true;
            }
        }
        return false;
    });
    rules_to_add.erase(it, rules_to_add.end());

    rules_to_add.insert(rules_to_add.end(),
                        other.rules_to_add.begin(),
                        other.rules_to_add.end());
    rules_to_delete.insert(rules_to_delete.end(),
                           other.rules_to_delete.begin(),
                           other.rules_to_delete.end());
    return *this;
}

std::ostream& operator<<(std::ostream& os, const InterceptorDiff& diff)
{
    os<<"+"<<diff.rules_to_add.size()
      <<" -"<<diff.rules_to_delete.size();
    return os;
}

void InterceptorManager::createInterceptor(DomainPathPtr path)
{
    auto switch_id = path->source->rule->sw()->id();
    auto interceptor = new_interceptor(path);
    interceptor_map_[switch_id].insert(interceptor);
    path->set_interceptor(interceptor);

    //std::cout<<"[Interceptor] New "<<interceptor<<std::endl;

    // Add interceptors
    InterceptorDiff new_diff;
    new_diff.rules_to_add.push_back(path->interceptor);
    diff_ += std::move(new_diff);
}
void InterceptorManager::deleteInterceptor(DomainPathPtr path)
{
    // Delete interceptors
    InterceptorDiff new_diff;
    new_diff.rules_to_delete.push_back(path->interceptor);
    diff_ += std::move(new_diff);

    //std::cout<<"[Interceptor] Delete "<<path->interceptor_rule_<<std::endl;

    auto switch_id = path->source->rule->sw()->id();
    interceptor_map_[switch_id].erase(path->interceptor_rule_);
    path->set_interceptor(RulePtr(nullptr));
}

InterceptorDiff InterceptorManager::popInterceptorDiff()
{
    auto diff = std::move(diff_);
    diff_.clear();
    return diff;
}

std::string InterceptorManager::diffToString() const
{
    std::stringstream ss;
    ss << diff_;
    return ss.str();
}

RulePtr InterceptorManager::new_interceptor(DomainPathPtr path) const
{
    auto priority = get_priority(path);
    Cookie cookie = 0xa000 + path->id;
    auto domain = path->source_domain;

    auto rule = std::make_shared<Rule>(
        RuleType::SOURCE, path->source->rule->sw(), nullptr,
        priority, cookie, std::move(domain), Actions::tableAction(1u));
    return rule;
}

Priority InterceptorManager::get_priority(DomainPathPtr path) const
{
    Priority priority = 1;

    auto it = interceptor_map_.find(path->source->rule->sw()->id());
    if (it != interceptor_map_.end()) {
        for (const auto &interceptor : it->second) {
            auto header = interceptor->domain().header();
            for (auto &bit_space: header.getBitSpace()) {
                for (auto &diff: bit_space.difference) {
                    if (domain_above_mask(path->source_domain, diff)) {
                        priority = std::max(
                            (unsigned) priority,
                            interceptor->priority() + 1u);
                    }
                }
            }
        }
    }
    return priority;
}

bool InterceptorManager::domain_above_mask(const NetworkSpace& domain,
                                           const BitMask& mask) const
{
    bool res = false;
    for (auto& bit_space: domain.header().getBitSpace()) {
        res |= (mask <= bit_space.mask);
        if (res) return true;
    }
    return res;
}
