#pragma once

#include "Visitor.hpp"
#include "../Controller.hpp"
#include "../Types.hpp"
#include "../openflow/Parser.hpp"

namespace pipeline {

class MessageHandler : public Visitor<fluid_msg::of13::FlowMod>,
                       public Visitor<fluid_msg::of13::PacketOut>,
                       public Visitor<fluid_msg::of13::PacketIn>,
                       public Visitor<fluid_msg::of13::MultipartReplyFlow>,
                       public Visitor<fluid_msg::OFMsg>
{
public:
    MessageHandler(ConnectionId id, Controller& controller):
        connection_id_(id), ctrl_(controller) {}

    // Controller messages
    Action visit(fluid_msg::of13::FlowMod&) override;
    Action visit(fluid_msg::of13::PacketOut&) override;

    // Switch messages
    Action visit(fluid_msg::of13::PacketIn&) override;
    Action visit(fluid_msg::of13::MultipartReplyFlow&) override;

    // Default
    Action visit(fluid_msg::OFMsg&) override;

private:
    ConnectionId connection_id_;
    Controller& ctrl_;

    RuleStatsFields get_rule_stats(const FlowStatsCommon& flow_stats) const;
    //PortStatsFields get_port_stats();
};

} // namespace pipeline
