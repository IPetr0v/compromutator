#pragma once

#include "../Types.hpp"
#include "Mapping.hpp"
#include "../NetworkSpace.hpp"
#include "../Detector.hpp"
#include "../proxy/Proxy.hpp"

#include <fluid/ofcommon/msg.hh>
#include <fluid/of10/openflow-10.h>
#include <fluid/of13/openflow-13.h>
#include <fluid/of13msg.hh>

#include <bitset>

// TODO: add openflow namespace
using namespace fluid_msg;

class Parser
{
    class ActionsBaseBridge;
public:
    template<class Message>
    static RuleInfo getRuleInfo(SwitchId switch_id, Message& message) {
        auto table_id = message.table_id();
        auto priority = message.priority();
        auto cookie = message.cookie();
        auto match = get_match(message.match());
        auto actions = get_actions(message.instructions());

        return {switch_id, table_id, priority, cookie,
                std::move(match), std::move(actions)};
    }

    static of13::FlowMod getFlowMod(RuleInfo rule);
    static of13::MultipartRequestFlow getMultipartRequestFlow(RuleInfo rule);

protected:
    // TODO: rewrite fluid_msg so we do not need to copy match (add const)
    static Match get_match(of13::Match match);
    static of13::Match get_of_match(const Match& match);

    static ActionsBase get_actions(of13::InstructionSet instructions);
    static ActionsBaseBridge get_apply_actions(of13::ApplyActions* actions);
    static Transfer get_transfer(const of13::SetFieldAction* action);
    static of13::InstructionSet get_of_instructions(ActionsBase actions);
};

// Messages that contain rule information
template RuleInfo Parser::getRuleInfo(SwitchId, of13::FlowMod&);
template RuleInfo Parser::getRuleInfo(SwitchId, of13::FlowStats&);
