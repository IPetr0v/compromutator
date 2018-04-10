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
        // Add switch to the detector
        detector_.addSwitch(info);

        // Save switch
        connection_map_.emplace(info.id, connection_id);
        switch_map_.emplace(connection_id, std::move(info));
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
        } else {
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
    const std::string& value = lldp.chassis_id->value;
    switch (lldp.chassis_id->type) {
    case proto::LLDP::Tlv::ChassisId::MAC:
        return std::stoi(value, nullptr, 16);
    case proto::LLDP::Tlv::ChassisId::LOCAL:
        // Ryu controller LLDP
        if (value.find("dpid:") != std::string::npos && value.length() > 5) {
            return std::stoi(value.substr(5, value.length() - 5), nullptr, 16);
        }
        else {
            throw std::invalid_argument("Wrong local Chassis Id");
        }
    default:
        throw std::invalid_argument("Unsupported Chassis Id");
    }
}

PortId LinkDiscovery::get_port_id(const proto::LLDP& lldp) const
{
    const std::string& value = lldp.port_id->value;
    switch (lldp.port_id->type) {
    case proto::LLDP::Tlv::PortId::COMPONENT:
        return std::stoi(value, nullptr, 10);
    case proto::LLDP::Tlv::PortId::LOCAL:
        // OpenDayLight controller LLDP
        return std::stoi(value, nullptr, 10);
    default:
        throw std::invalid_argument("Unsupported Port Id");
    }
}

void RuleManager::installRule(const RuleInfo& info)
{

}

void RuleManager::deleteRule(const RuleInfo& info)
{

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
