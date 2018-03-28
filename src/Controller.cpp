#include "Controller.hpp"

#include "Detector.hpp"
#include "openflow/Parser.hpp"

using namespace fluid_msg;

Controller::Controller(Sender sender, Detector& detector):
    sender_(sender), detector_(detector)
{

}

const SwitchInfo* Controller::getSwitch(ConnectionId connection_id) const
{
    auto switch_iter = switch_map_.find(connection_id);
    if (switch_map_.end() != switch_iter) {
        return &switch_iter->second;
    }
    return nullptr;
}

void Controller::addSwitch(ConnectionId connection_id, SwitchInfo&& info)
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

void Controller::deleteSwitch(ConnectionId connection_id)
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

/*void Controller::getPortDesc(ConnectionId id)
{
    auto xid = xid_factory_.getXid();
    of13::MultipartRequestPortDescription port_desc(xid, 0u);
    send_to_switch(id, port_desc);
}*/

void Controller::getRuleStats(RequestId request_id, const RuleInfo& info)
{
    auto result = get_connection_id(info.switch_id);
    if (result.second) {
        // Save request id
        auto xid = xid_factory_.getXid();
        request_id_map_.emplace(xid, request_id);

        // Send rule stats request
        auto connection_id = result.first;
        auto request_flow = Parser::getMultipartRequestFlow(info);
        send_to_switch(connection_id, request_flow);
    }
    else {
        std::cerr << "Controller error: "
                  << "Sending rule stats request to an offline switch"
                  << std::endl;
    }
}

void Controller::getPortStats(RequestId request_id,
                              SwitchId switch_id, PortId port_id)
{
    auto result = get_connection_id(switch_id);
    if (result.second) {
        // Save request id
        auto xid = xid_factory_.getXid();
        request_id_map_.emplace(xid, request_id);

        // Send port stats request
        auto connection_id = result.first;
        of13::MultipartRequestPortStats request_port_stats(xid, 0u, port_id);
        send_to_switch(connection_id, request_port_stats);
    }
    else {
        std::cerr << "Controller error: "
                  << "Sending port stats request to an offline switch"
                  << std::endl;
    }
}

void Controller::installRule(const RuleInfo& info)
{

}

void Controller::deleteRule(const RuleInfo& info)
{

}

std::pair<RequestId, bool> Controller::popRequestId(uint32_t xid)
{
    auto it = request_id_map_.find(xid);
    if (request_id_map_.end() != it) {
        auto request_id = it->second;
        request_id_map_.erase(it);
        return std::make_pair(request_id, true);
    }
    return std::make_pair((uint32_t)-1, false);
}

std::pair<ConnectionId, bool> Controller::get_connection_id(SwitchId switch_id)
{
    auto it = connection_map_.find(switch_id);
    return connection_map_.end() != it
           ? std::make_pair(it->second, true)
           : std::make_pair((ConnectionId)-1, false);
}

void Controller::send_to_controller(ConnectionId id, OFMsg& message)
{
    RawMessage raw_message(message);
    sender_.send(id, Destination::TO_CONTROLLER, std::move(raw_message));
}

void Controller::send_to_switch(ConnectionId id, OFMsg& message)
{
    RawMessage raw_message(message);
    sender_.send(id, Destination::TO_SWITCH, std::move(raw_message));
}
