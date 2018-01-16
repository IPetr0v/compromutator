#pragma once

#include "Types.hpp"
#include "../network/Rule.hpp"
#include "../network/Network.hpp"

#include <cstdint>

class Controller
{
public:
    // TODO: CRITICAL - xid from the real controller may be equal to detectors!

    void getRuleStats(RequestId xid, const RulePtr rule);
    void getPortStats(RequestId xid, const PortPtr port);

    void installRule(const RulePtr rule);
    void deleteRule(const RulePtr rule);
};
