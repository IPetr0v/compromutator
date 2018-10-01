#pragma once

#include "PathScan.hpp"
#include "../Types.hpp"
#include "../NetworkSpace.hpp"
#include "../network/Rule.hpp"

#include <algorithm>
#include <list>

struct Interceptor;
using InterceptorPtr = std::list<Interceptor>::iterator;

struct Interceptor
{
    Interceptor(DomainPathPtr path, Priority priority):
        descriptor(InterceptorPtr(nullptr))
    {
        Cookie cookie = 0xa000 + path->id;//(path->id << 16) | 0x1337;
        auto domain = path->source_domain;
        this->rule = new Rule(
            RuleType::SOURCE, path->source->rule->sw(), nullptr,
            priority, cookie, std::move(domain), Actions::noActions());
    }
    ~Interceptor() {
        delete this->rule;
    }

    RuleInfo info() const {
        auto headers = rule->match().header().getBitSpace();
        assert(headers.size() == 1u);
        auto switch_id = rule->sw()->id();
        auto table_id = rule->table() ? rule->table()->id() : (TableId) 0;
        auto cookie = rule->cookie();//0x1337;
        auto priority = rule->priority();
        auto match = Match(rule->match().inPort(), std::move(headers[0].mask));
        auto actions = ActionsBase::tableAction(1u);
        return RuleInfo(switch_id, table_id, priority, cookie, match, actions);
    }

    InterceptorPtr descriptor;
    RulePtr rule;
};

struct InterceptorDiff
{
    std::vector<RulePtr> rules_to_add;
    std::vector<RulePtr> rules_to_delete;

    InterceptorDiff& operator+=(const InterceptorDiff& other);
    bool empty() const {return rules_to_add.empty() && rules_to_delete.empty();}
    void clear() {
        rules_to_add.clear();
        rules_to_delete.clear();
    }
    friend std::ostream& operator<<(std::ostream& os,
                                    const InterceptorDiff& diff);

    // TODO: move parsing to the new InterceptorDiff (with graph and priorities)
    std::vector<RuleInfo> getRulesToAdd() const;
    std::vector<RuleInfo> getRulesToDelete() const;
    std::vector<RuleInfo> getRules(std::vector<RulePtr> original_rules) const;
};

struct InterceptorDiff_
{
    std::list<RuleInfo> rules_to_add;
    std::list<RuleInfo> rules_to_delete;

    bool empty() {return rules_to_add.empty() && rules_to_delete.empty();}
    void clear() {
        rules_to_add.clear();
        rules_to_delete.clear();
    }
};

struct InterceptorGraph
{

};

class InterceptorManager
{
public:
    void createInterceptor(DomainPathPtr path) {
        Cookie cookie = 0xa000 + path->id;//(path->id << 16) | 0x1337;
        auto domain = path->source_domain;
        Priority priority = get_priority(domain);
        auto rule = new Rule(
            RuleType::SOURCE, path->source->rule->sw(), nullptr,
            priority, cookie, std::move(domain), Actions::noActions());
        path->source_interceptor = rule;
        rules_.push_back(rule);

        // Add interceptors
        InterceptorDiff new_diff;
        new_diff.rules_to_add.push_back(rule);
        diff_ += new_diff;

        /*Priority priority = get_priority(path->source_domain);
        auto it = interceptors_.emplace(interceptors_.end(), path, priority);
        it->descriptor = it;
        path->source_interceptor = it;*/
    }
    void deleteInterceptor(DomainPathPtr path) {
        // Delete interceptors
        auto rule = path->source_interceptor;
        InterceptorDiff new_diff;
        new_diff.rules_to_delete.push_back(rule);
        diff_ += new_diff;

        // TODO: Delete rule and remove it from rule list
        ///delete path->source_interceptor;

    }

    InterceptorDiff popInterceptorDiff() {
        auto diff = std::move(diff_);
        diff_.clear();
        return diff;
    }

    std::string diffToString() const {
        std::stringstream ss;
        ss << diff_;
        return ss.str();
    }

private:
    //PathScan& path_scan_;
    //std::map<SwitchId, InterceptorGraph> interceptors_;
    std::list<RulePtr> rules_;
    std::list<Interceptor> interceptors_;
    InterceptorDiff diff_;

    Priority get_priority(const NetworkSpace& domain) const {
        Priority priority = 1;
        for (auto rule : rules_) {
            auto header = rule->match().header();
            for (auto& bit_space: header.getBitSpace()) {
                for (auto& diff: bit_space.difference) {
                    if (domain_above_mask(domain, diff)) {
                        priority = std::max(
                            (unsigned)priority,
                            rule->priority() + 1u);
                    }
                }
            }
        }
        return priority;
    }

    bool domain_above_mask(const NetworkSpace& domain, const BitMask& mask) const {
        bool res = false;
        for (auto& bit_space: domain.header().getBitSpace()) {
            res |= (mask <= bit_space.mask);
            if (res) return true;
        }
        return res;
    }
};
