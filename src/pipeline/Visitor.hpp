#pragma once

#include "../Proto.hpp"
#include "../openflow/MessageDispatcher.hpp"

namespace pipeline {

enum class Action {
    FORWARD, ENQUEUE, DROP
};

using Dispatcher = MessageDispatcher<Action, fluid_msg::OFMsg>;
template<class Message> using Visitor = Dispatcher::Visitor<Message>;

using ChangerDispatcher = MessageDispatcher<RawMessage, fluid_msg::OFMsg>;
template<class Message> using Changer = ChangerDispatcher::Visitor<Message>;

class HandlerBase {
public:
    bool isLLDP(const fluid_msg::of13::FlowMod &flow_mod) {
        return flow_mod.match().eth_type() &&
               flow_mod.match().eth_type()->value() ==
               proto::Ethernet::TYPE::LLDP;
    };
};

} // namespace pipeline
