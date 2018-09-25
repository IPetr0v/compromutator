#pragma once

#include "PathScan.hpp"

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
    void addDomainPath(DomainPathPtr path) {
        //auto in_port = path->source_domain.inPort();
        //auto bit_space = path->source_domain.header().getBitSpace();
    }
    void deleteDomainPath(DomainPathPtr path) {

    }

    InterceptorDiff_ popInterceptorDiff() {
        auto diff = std::move(diff_);
        diff_.clear();
        return diff;
    }

private:
    //PathScan& path_scan_;
    std::map<SwitchId, InterceptorGraph> interceptors_;
    InterceptorDiff_ diff_;
};
