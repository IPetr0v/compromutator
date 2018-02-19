#pragma once

#include "Types.hpp"
#include "Mapping.hpp"
#include "../NetworkSpace.hpp"
#include "../Detector.hpp"
#include "../proxy/Proxy.hpp"

#include <fluid/ofcommon/msg.hh>
#include <fluid/of10/openflow-10.h>
#include <fluid/of13/openflow-13.h>
#include <fluid/of13msg.hh>

#include <bitset>

using namespace fluid_msg;

class Parser
{
public:
    RuleInfo getRuleInfo(SwitchId switch_id, of13::FlowMod& flow_mod) const {
        auto table_id = flow_mod.table_id();
        auto priority = flow_mod.priority();
        auto match = get_match(flow_mod.match());
        auto actions = get_actions(flow_mod.instructions());

        return {switch_id, table_id, priority,
                std::move(match), std::move(actions)};
    }

    of13::FlowMod getFlowMod(RuleInfo rule) const {
        of13::FlowMod flow_mod;

        flow_mod.table_id(rule.table_id);
        flow_mod.priority(rule.priority);
        flow_mod.match(get_of_match(rule.match));
        flow_mod.instructions(get_instructions(rule.actions));

        return std::move(flow_mod);
    }

private:
    class ActionsBaseBridge;

    // TODO: rewrite fluid_msg so we do not need to copy match (add const)
    Match get_match(of13::Match match) const;
    of13::Match get_of_match(const Match& match) const;

    ActionsBase get_actions(of13::InstructionSet instructions) const;
    ActionsBaseBridge get_apply_actions(of13::ApplyActions* actions) const;
    Transfer get_transfer(const of13::SetFieldAction* action) const;
    of13::InstructionSet get_instructions(ActionsBase actions) const;
};
