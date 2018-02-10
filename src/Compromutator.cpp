#include "Compromutator.hpp"

#include <chrono>

void Compromutator::run()
{
    is_running_ = true;
    while (is_running_) {
        auto status = alarm_->wait(std::chrono::milliseconds(100));
        if (status == Alarm::Status::TIMEOUT) {
            on_timeout();
        }
        else {
            if (proxy_.eventsExist()) {
                on_proxy_event();
            }
        }
    }
}

void Compromutator::on_timeout()
{

}

void Compromutator::on_proxy_event()
{
    auto event = proxy_.getEvent();
    switch (event->type) {
    case EventType::CONNECTION: {
        auto connection_event = ConnectionEvent::pointerCast(event);
        if (connection_event->type == ConnectionEventType::NEW) {

        }
        else {

        }
        break;
    }
    case EventType::MESSAGE: {
        auto message_event = MessageEvent::pointerCast(event);
        if (message_event->origin == Origin::FROM_CONTROLLER) {
            on_controller_message(
                message_event->connection_id, message_event->message
            );
        }
        else {
            on_switch_message(
                message_event->connection_id, message_event->message
            );
        }
        break;
    }
    }
}

void Compromutator::on_detector_event()
{
    
}

void Compromutator::on_controller_message(ConnectionId connection_id,
                                          Message message)
{
    using namespace fluid_msg;
    switch (message.type) {
    case of13::OFPT_HELLO:
        std::cout << "proxying HELLO from the controller" << std::endl;
        proxy_.sendToSwitch(connection_id, message);
        break;
    case of13::OFPT_FEATURES_REQUEST:
        std::cout << "proxying FEATURES_REQUEST from the controller" << std::endl;
        proxy_.sendToSwitch(connection_id, message);
        break;
    case of13::OFPT_FLOW_MOD:
        break;
        /*case of13::OFPT_PORT_MOD:
            break;
        case of13::OFPT_METER_MOD:
            break;
        case of13::OFPT_GROUP_MOD:
            break;
        case of13::OFPT_PACKET_OUT:
            break;
        case of13::OFPT_BARRIER_REQUEST:
            break;
        case of13::OFPT_GET_CONFIG_REQUEST:
            break;
        case of13::OFPT_QUEUE_GET_CONFIG_REQUEST:
            break;
        case of13::OFPT_ROLE_REQUEST:
            break;
        case of13::OFPT_GET_ASYNC_REQUEST:
            break;
        case of13::OFPT_MULTIPART_REQUEST:
            break;*/
    default:
        proxy_.sendToSwitch(connection_id, message);
        break;
    }

}

void Compromutator::on_switch_message(ConnectionId connection_id,
                                      Message message)
{
    using namespace fluid_msg;
    switch (message.type) {
    case of13::OFPT_HELLO:
        std::cout << "proxying HELLO from the switch" << std::endl;
        proxy_.sendToController(connection_id, message);
        break;
    case of13::OFPT_FEATURES_REPLY:
        std::cout << "proxying FEATURES_REPLY from the switch" << std::endl;
        proxy_.sendToController(connection_id, message);
        break;
        /*case of13::OFPT_ERROR:
            break;
        case of13::OFPT_GET_CONFIG_REPLY:
            break;
        case of13::OFPT_EXPERIMENTER:
            break;
        case of13::OFPT_PACKET_IN:
            break;
        case of13::OFPT_FLOW_REMOVED:
            break;
        case of13::OFPT_PORT_STATUS:
            break;
        case of13::OFPT_BARRIER_REPLY:
            break;
        case of13::OFPT_QUEUE_GET_CONFIG_REPLY:
            break;
        case of13::OFPT_ROLE_REPLY:
            break;
        case of13::OFPT_GET_ASYNC_REPLY:
            break;
        case of13::OFPT_MULTIPART_REPLY:
            break;*/
    default:
        proxy_.sendToController(connection_id, message);
        break;
    }
}
