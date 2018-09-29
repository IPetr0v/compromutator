#pragma once

#include "Visitor.hpp"
#include "../Controller.hpp"
#include "../Types.hpp"

namespace pipeline {

struct MessageChanger : public Changer<fluid_msg::of13::FlowMod>,
                        public Changer<fluid_msg::of13::MultipartRequestFlow>,
                        public Changer<fluid_msg::of13::MultipartReplyFlow>,
                        public Changer<fluid_msg::OFMsg>,
                        public HandlerBase
{
    // Controller messages
    RawMessage visit(fluid_msg::of13::FlowMod& flow_mod) override;
    RawMessage visit(fluid_msg::of13::MultipartRequestFlow&) override;

    // Switch messages
    RawMessage visit(fluid_msg::of13::MultipartReplyFlow&) override;

    // Default
    RawMessage visit(fluid_msg::OFMsg&) override;
};

} // namespace pipeline
