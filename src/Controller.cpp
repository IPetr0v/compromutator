#include "Controller.hpp"

#include "Detector.hpp"
#include "openflow/Parser.hpp"

using namespace fluid_msg;

const SwitchInfo* SwitchManager::getSwitch(ConnectionId connection_id) const
{
    auto switch_iter = switch_map_.find(connection_id);
    if (switch_map_.end() != switch_iter) {
        return &switch_iter->second;
    }
    return nullptr;
}

void SwitchManager::addSwitch(ConnectionId connection_id, SwitchInfo&& info)
{
    auto connection_iter = connection_map_.find(info.id);
    if (connection_map_.end() == connection_iter) {
        std::cout << "Switch dpid=" << info.id << " connected" << std::endl;

        // Add switch to the detector
        detector_.addSwitch(info);

        // Save switch
        connection_map_.emplace(info.id, connection_id);
        switch_map_.emplace(connection_id, std::move(info));

        // Install auxiliary rules

    }
    else {
        std::cerr << "Controller error: Already existing switch"
                  << std::endl;
    }
}

void SwitchManager::deleteSwitch(ConnectionId connection_id)
{
    auto switch_iter = switch_map_.find(connection_id);
    if (switch_map_.end() != switch_iter) {
        auto switch_id = switch_iter->second.id;
        std::cout << "Switch dpid=" << switch_id << " disconnected" << std::endl;

        // Delete switch from the detector
        detector_.deleteSwitch(switch_id);

        // Delete switch
        connection_map_.erase(switch_id);
        switch_map_.erase(switch_iter);
    }
}

std::pair<ConnectionId, bool>
SwitchManager::getConnectionId(SwitchId switch_id) const
{
    auto it = connection_map_.find(switch_id);
    return connection_map_.end() != it
           ? std::make_pair(it->second, true)
           : std::make_pair((ConnectionId)-1, false);
}

void LinkDiscovery::handleLLDP(ConnectionId connection_id, PortId in_port,
                               const proto::LLDP& lldp)
{
    try {
        auto src_switch_id = get_switch_id(lldp);
        auto dst_switch_id = switch_manager_.getSwitch(connection_id)->id;

        // Check source switch
        auto result = switch_manager_.getConnectionId(src_switch_id);
        if (result.second) {
            auto src_port_id = get_port_id(lldp);
            auto dst_port_id = in_port;
            detector_.addLink({src_switch_id, src_port_id},
                              {dst_switch_id, dst_port_id});
        }
        else {
            std::cerr << "LinkDiscovery error: "
                      << "Source switch is offline"
                      << std::endl;
        }
    }
    catch (const std::invalid_argument& error) {
        std::cerr << "LinkDiscovery error: " << error.what() << std::endl;
    }
}

SwitchId LinkDiscovery::get_switch_id(const proto::LLDP& lldp) const
{
    // TODO: check for hex
    switch (lldp.chassis_id->type) {
    case proto::LLDP::Tlv::ChassisId::MAC:
        return lldp.chassis_id->decimalValue();
    case proto::LLDP::Tlv::ChassisId::LOCAL: {
        // Ryu controller LLDP
        auto value = lldp.chassis_id->stringValue();
        if (value.find("dpid:") != std::string::npos && value.length() > 5) {
            return std::stoull(value.substr(5, value.length() - 5), nullptr, 16);
        } else {
            throw std::invalid_argument("Wrong Local Chassis Id");
        }
    }
    default:
        throw std::invalid_argument("Unsupported Chassis Id");
    }
}

PortId LinkDiscovery::get_port_id(const proto::LLDP& lldp) const
{
    switch (lldp.port_id->type) {
    case proto::LLDP::Tlv::PortId::COMPONENT:
        return lldp.port_id->decimalValue();
    case proto::LLDP::Tlv::PortId::LOCAL:
        // OpenDayLight controller LLDP
        return lldp.port_id->decimalValue();
    default:
        throw std::invalid_argument("Unsupported Port Id");
    }
}

void RuleManager::installRule(const RuleInfo& info)
{
    auto flow_mod = Parser::getFlowMod(info);
    flow_mod.command(of13::OFPFC_ADD);
    send_flow_mod(info.switch_id, flow_mod);
}

void RuleManager::deleteRule(const RuleInfo& info)
{
    auto flow_mod = Parser::getFlowMod(info);
    flow_mod.command(of13::OFPFC_DELETE);
    send_flow_mod(info.switch_id, flow_mod);
}

void RuleManager::deleteRulesByCookie(const RuleInfo& info)
{
    of13::FlowMod flow_mod;
    flow_mod.cookie(info.cookie);
    flow_mod.cookie_mask((uint64_t)-1);
    flow_mod.command(of13::OFPFC_DELETE);
    send_flow_mod(info.switch_id, flow_mod);
}

void RuleManager::initSwitch(SwitchId id)
{
    // TODO: remove this (should be done by the controller)
    // Clear rules
    for (auto table_id : {0, 1}) {
        of13::FlowMod flow_mod_clear;
        flow_mod_clear.table_id(table_id);
        flow_mod_clear.command(of13::OFPFC_DELETE);
        send_flow_mod(id, flow_mod_clear);
    }

    // Install traverse rule
    of13::FlowMod flow_mod;
    flow_mod.command(of13::OFPFC_ADD);
    flow_mod.priority(ZERO_PRIORITY);
    flow_mod.xid(0);
    flow_mod.table_id(0);
    flow_mod.cookie(0xDEF);
    of13::GoToTable goto_table(1u);
    flow_mod.add_instruction(goto_table);
    send_flow_mod(id, flow_mod);
}

void RuleManager::send_flow_mod(SwitchId switch_id,
                                fluid_msg::of13::FlowMod& flow_mod)
{
    auto result = switch_manager_.getConnectionId(switch_id);
    if (result.second) {
        // Save request id
        auto xid = xid_manager_.getXid();
        flow_mod.xid(xid);

        // Send rule stats request
        auto connection_id = result.first;
        sender_.send(connection_id, Destination::TO_SWITCH, flow_mod);
    }
    else {
        std::cerr << "Controller error: "
                  << "Sending flow mod to an offline switch"
                  << std::endl;
    }
}

void StatsQuerier::getRuleStats(RequestId request_id, const RuleInfo& info)
{
    auto result = switch_manager_.getConnectionId(info.switch_id);
    if (result.second) {
        // Save request id
        auto xid = xid_manager_.getXid();
        request_id_map_.emplace(xid, request_id);

        // Send rule stats request
        auto connection_id = result.first;
        auto request_flow = Parser::getMultipartRequestFlow(info);
        sender_.send(connection_id, Destination::TO_SWITCH, request_flow);
    }
    else {
        std::cerr << "Controller error: "
                  << "Sending rule stats request to an offline switch"
                  << std::endl;
    }
}

void StatsQuerier::getPortStats(RequestId request_id,
                              SwitchId switch_id, PortId port_id)
{
    auto result = switch_manager_.getConnectionId(switch_id);
    if (result.second) {
        // Save request id
        auto xid = xid_manager_.getXid();
        request_id_map_.emplace(xid, request_id);

        // Send port stats request
        auto connection_id = result.first;
        of13::MultipartRequestPortStats request_port_stats(xid, 0u, port_id);
        sender_.send(connection_id, Destination::TO_SWITCH, request_port_stats);
    }
    else {
        std::cerr << "Controller error: "
                  << "Sending port stats request to an offline switch"
                  << std::endl;
    }
}

std::pair<RequestId, bool> StatsQuerier::popRequestId(uint32_t xid)
{
    auto it = request_id_map_.find(xid);
    if (request_id_map_.end() != it) {
        auto request_id = it->second;
        request_id_map_.erase(it);
        return std::make_pair(request_id, true);
    }
    return std::make_pair((uint32_t)-1, false);
}

Controller::Controller(std::shared_ptr<Alarm> alarm, Sender sender):
    detector(alarm),
    xid_manager(),
    switch_manager(detector),
    link_discovery(switch_manager, detector),
    rule_manager(xid_manager, switch_manager, sender),
    stats_manager(xid_manager, switch_manager, sender)
{

}
