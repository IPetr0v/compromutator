#pragma once

#include "../openflow/MessageDispatcher.hpp"

namespace pipeline {

enum class Action {FORWARD, ENQUEUE, DROP};

using Dispatcher = MessageDispatcher<Action, fluid_msg::OFMsg>;
template<class Message> using Visitor = Dispatcher::Visitor<Message>;

using ChangerDispatcher = MessageDispatcher<RawMessage, fluid_msg::OFMsg>;
template<class Message> using Changer = ChangerDispatcher::Visitor<Message>;

} // namespace pipeline
