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
SwitchManager::getConnectionId(SwitchId switch_id)
{
    auto it = connection_map_.find(switch_id);
    return connection_map_.end() != it
           ? std::make_pair(it->second, true)
           : std::make_pair((ConnectionId)-1, false);
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
    link_discovery(detector),
    rule_manager(xid_manager, switch_manager, sender),
    stats_manager(xid_manager, switch_manager, sender)
{

}
