#pragma once

#include "Visitor.hpp"
#include "../Controller.hpp"
#include "../Types.hpp"

namespace pipeline {

class MessagePostprocessor : public Postprocessor<fluid_msg::of13::FlowMod>,
                             public Postprocessor<fluid_msg::OFMsg>,
                             public HandlerBase
{
public:
    explicit MessagePostprocessor(ConnectionId id, Controller& ctrl):
        connection_id_(id), ctrl_(ctrl) {}

    void visit(fluid_msg::of13::FlowMod&) override;

    // Default
    void visit(fluid_msg::OFMsg&) override;

private:
    ConnectionId connection_id_;
    Controller& ctrl_;

};

} // namespace pipeline
