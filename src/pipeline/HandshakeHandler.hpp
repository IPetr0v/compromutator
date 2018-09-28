#pragma once

#include "Visitor.hpp"
#include "../Controller.hpp"
#include "../Types.hpp"

namespace pipeline {

class HandshakeHandler : public Visitor<fluid_msg::of13::Hello>,
                         public Visitor<fluid_msg::of13::Error>,
                         public Visitor<fluid_msg::of13::EchoRequest>,
                         public Visitor<fluid_msg::of13::EchoReply>,
                         public Visitor<fluid_msg::of13::FeaturesRequest>,
                         public Visitor<fluid_msg::of13::FeaturesReply>,
                         public Visitor<fluid_msg::of13::MultipartRequestPortDescription>,
                         public Visitor<fluid_msg::of13::MultipartReplyPortDescription>,
                         public Visitor<fluid_msg::OFMsg>
{
    struct SwitchInfoStatus {
        SwitchInfoStatus(): features(false), ports(false) {}
        bool features, ports;
        bool isFull() const {return features and ports;}
    };

public:
    HandshakeHandler(ConnectionId id, Controller& controller):
        connection_id_(id), switch_manager_(controller.switch_manager),
        rule_manager_(controller.rule_manager), is_established_(false) {}

    bool established() const {return is_established_;}

    Action visit(fluid_msg::of13::Hello&) override;
    Action visit(fluid_msg::of13::Error&) override;
    Action visit(fluid_msg::of13::EchoRequest&) override;
    Action visit(fluid_msg::of13::EchoReply&) override;
    Action visit(fluid_msg::of13::FeaturesRequest&) override;
    Action visit(fluid_msg::of13::FeaturesReply&) override;
    Action visit(fluid_msg::of13::MultipartRequestPortDescription&) override;
    Action visit(fluid_msg::of13::MultipartReplyPortDescription&) override;
    Action visit(fluid_msg::OFMsg&) override;

private:
    ConnectionId connection_id_;
    SwitchManager& switch_manager_;
    RuleManager& rule_manager_;

    bool is_established_;
    SwitchInfoStatus info_status_;
    SwitchInfo info_;

    void try_establish();
};

} // namespace pipeline
