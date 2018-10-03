#pragma once

#include "Node.hpp"
#include "PathScan.hpp"
#include "../Types.hpp"
#include "../NetworkSpace.hpp"
#include "../network/Rule.hpp"

#include <algorithm>
#include <list>
#include <unordered_map>
#include <unordered_set>

struct InterceptorDiff
{
    std::vector<RuleInfoPtr> rules_to_add;
    std::vector<RuleInfoPtr> rules_to_delete;

    InterceptorDiff& operator+=(const InterceptorDiff& other);
    bool empty() const {return rules_to_add.empty() && rules_to_delete.empty();}
    void clear() {
        rules_to_add.clear();
        rules_to_delete.clear();
    }
    friend std::ostream& operator<<(std::ostream& os,
                                    const InterceptorDiff& diff);
};

class InterceptorManager
{
    using InterceptorTable = std::unordered_set<
        RulePtr, Rule::PtrHash, Rule::PtrEqualityComparator>;
public:
    void createInterceptor(DomainPathPtr path);
    void deleteInterceptor(DomainPathPtr path);
    InterceptorDiff popInterceptorDiff();

    std::string diffToString() const;

private:
    std::unordered_map<SwitchId, InterceptorTable> interceptor_map_;
    InterceptorDiff diff_;

    RulePtr new_interceptor(DomainPathPtr path) const;
    Priority get_priority(DomainPathPtr path) const;
    bool domain_above_mask(const NetworkSpace& domain,
                           const BitMask& mask) const;
};
