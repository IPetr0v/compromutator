#include "MessageHandler.hpp"

namespace pipeline {

using namespace fluid_msg;

Action MessageHandler::visit(of13::FlowMod& flow_mod)
{
    auto switch_id = ctrl_.switch_manager.getSwitch(connection_id_)->id;
    auto rule_info = Parser::getRuleInfo(switch_id, flow_mod);
    switch (flow_mod.command()) {
    case of13::OFPFC_ADD:
        // Do not consider LLDP rules
        if (not isLLDP(flow_mod)) {
            ctrl_.detector.addRule(switch_id, rule_info);
        }
        break;
    case of13::OFPFC_MODIFY:
        // TODO: delete rule only by Domain (without Actions)
        ctrl_.detector.deleteRule(switch_id, rule_info);
        ctrl_.detector.addRule(switch_id, rule_info);
        break;
    case of13::OFPFC_MODIFY_STRICT:
        // TODO: implement modify strict
        std::cerr << "OFPFC_MODIFY_STRICT is not implemented" << std::endl;
        break;
    case of13::OFPFC_DELETE:
        ctrl_.detector.deleteRule(switch_id, rule_info);
        break;
    case of13::OFPFC_DELETE_STRICT:
        // TODO: implement delete strict
        std::cerr << "OFPFC_DELETE_STRICT is not implemented" << std::endl;
        break;
    default:
        std::cerr << "Unknown flow mod command" << std::endl;
        break;
    }
    return Action::ENQUEUE;
}

Action MessageHandler::visit(of13::PacketOut& packet_out)
{
    // TODO: compute path and add stats to the detector

    return Action::FORWARD;
}

Action MessageHandler::visit(of13::PacketIn& packet_in)
{
    // Check for LLDP
    try {
        auto lldp = proto::LLDP(packet_in.data(), packet_in.data_len());
        auto in_port = packet_in.match().in_port()->value();
        ctrl_.link_discovery.handleLLDP(connection_id_, in_port, lldp);
    }
    catch (const std::invalid_argument& error) {
        // Ignore the Packet-In message
    }
    return Action::FORWARD;
}

Action MessageHandler::visit(of13::MultipartReplyFlow& reply_flow)
{
    auto result = ctrl_.stats_manager.popRequestId(reply_flow.xid());
    if (result.second) {
        auto request_id = result.first;
        auto switch_id = ctrl_.switch_manager.getSwitch(connection_id_)->id;
        for (auto& flow_stats : reply_flow.flow_stats()) {
            auto rule_info = Parser::getRuleInfo(switch_id, flow_stats);
            ctrl_.detector.addRuleStats(request_id, rule_info,
                                        get_rule_stats(flow_stats));
        }

        // This message is not for the controller
        return Action::DROP;
    }
    return Action::FORWARD;
}

Action MessageHandler::visit(OFMsg&)
{
    return Action::FORWARD;
}

RuleStatsFields
MessageHandler::get_rule_stats(const FlowStatsCommon& flow_stats) const
{
    RuleStatsFields stats;
    stats.packet_count = flow_stats.packet_count();
    stats.byte_count = flow_stats.byte_count();
    return stats;
}

} // namespace pipeline
