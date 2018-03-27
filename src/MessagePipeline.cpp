#include "MessagePipeline.hpp"

#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pipeline {

using namespace fluid_msg;

Action HandshakeHandler::visit(fluid_msg::of13::Hello& hello)
{
    return Action::FORWARD;
}

Action HandshakeHandler::visit(of13::FeaturesReply& features_reply)
{
    info_.id = features_reply.datapath_id();
    info_.table_number = features_reply.n_tables();
    info_status_.features = true;
    return try_establish();
}

Action HandshakeHandler::visit(of13::MultipartReplyPortDescription& port_desc)
{
    for (const auto &port : port_desc.ports()) {
        info_.ports.emplace_back(port.port_no(), port.curr_speed());
    }
    info_status_.ports = true;
    return try_establish();
}

Action HandshakeHandler::visit(OFMsg& message)
{
    return try_establish();
}

Action HandshakeHandler::try_establish()
{
    if (not is_established_ && info_status_.isFull()) {
        controller_.addSwitch(connection_id_, std::move(info_));
        is_established_ = true;
        return Action::FORWARD;
    } else if (is_established_) {
        return Action::FORWARD;
    } else {
        return Action::ENQUEUE;
    }
}

Action MessageHandler::visit(of13::FlowMod& flow_mod)
{
    auto switch_id = controller_.getSwitch(connection_id_)->id;
    auto rule_info = Parser::getRuleInfo(switch_id, flow_mod);
    switch (flow_mod.command()) {
    case of13::OFPFC_ADD:
        detector_.addRule(switch_id, rule_info);
        break;
    case of13::OFPFC_MODIFY:
        // TODO: delete rule only by Domain (without Actions)
        detector_.deleteRule(switch_id, rule_info);
        detector_.addRule(switch_id, rule_info);
        break;
    case of13::OFPFC_MODIFY_STRICT:
        // TODO: implement modify strict
        std::cerr << "OFPFC_MODIFY_STRICT is not implemented" << std::endl;
        break;
    case of13::OFPFC_DELETE:
        detector_.deleteRule(switch_id, rule_info);
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

Action MessageHandler::visit(of13::MultipartReplyFlow& reply_flow)
{
    auto result = controller_.popRequestId(reply_flow.xid());
    if (result.second) {
        auto request_id = result.first;
        auto switch_id = controller_.getSwitch(connection_id_)->id;
        for (auto& flow_stats : reply_flow.flow_stats()) {
            auto rule_info = Parser::getRuleInfo(switch_id, flow_stats);
            detector_.addRuleStats(request_id, rule_info,
                                   get_rule_stats(flow_stats));
        }

        // This message is not for the controller
        return Action::DROP;
    }
    return Action::FORWARD;
}

Action MessageHandler::visit(OFMsg& message)
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

RawMessage MessageChanger::visit(of13::FlowMod& flow_mod)
{
    flow_mod.table_id(flow_mod.table_id() + 1);
    return {flow_mod};
}

RawMessage MessageChanger::visit(of13::MultipartRequestFlow& request_flow)
{
    request_flow.table_id(request_flow.table_id() + 1);
    return {request_flow};
}

RawMessage MessageChanger::visit(of13::MultipartReplyFlow& reply_flow)
{
    for (auto& flow_stats : reply_flow.flow_stats()) {
        flow_stats.table_id(flow_stats.table_id() - 1);
    }
    return {reply_flow};
}

void MessagePipeline::addConnection(ConnectionId id)
{
    auto handshake_it = handshake_pipelines_.find(id);
    auto handler_it = message_handlers_.find(id);
    if (handshake_pipelines_.end() == handshake_it &&
        message_handlers_.end() == handler_it) {
        handshake_pipelines_.emplace(id, HandshakePipeline(id, controller_));
    }
    else {
        std::cerr << "Pipeline error: Already existing connection" << std::endl;
    }
}

void MessagePipeline::deleteConnection(ConnectionId id)
{
    // TODO: implement connection deletion
}

void MessagePipeline::processMessage(Message message)
{
    // Check if the connection hasn't been established yet
    auto handshake_it = handshake_pipelines_.find(message.id);
    if (handshake_pipelines_.end() != handshake_it) {
        // Dispatch message
        auto& pipeline = handshake_it->second;
        auto pre_action = Dispatcher()(message.raw_message, pipeline.handler);

        // Process message
        switch (pre_action) {
        case Action::FORWARD:
            handle_message(message);
            break;
        case Action::ENQUEUE:
            pipeline.queue.push(message);
            break;
        case Action::DROP:
            break;
        }
        
        // Check connection status
        if (pipeline.handler.established()) {
            while (not pipeline.queue.empty()) {
                auto queued_message = pipeline.queue.front();
                pipeline.queue.pop();
                handle_message(queued_message);
            }
            
            // Delete handshake pipeline for the established connection
            handshake_pipelines_.erase(handshake_it);
        }
    }
    else {
        handle_message(message);
    }
}

void MessagePipeline::addBarrier()
{
    queue_.addBarrier();
}

void MessagePipeline::flushPipeline(ConnectionId id)
{
    // TODO: check pipeline for emptiness
    for (auto message : queue_.pop()) {
        forward_message(message);
    }
}

void MessagePipeline::handle_message(Message message)
{
    auto handler_it = message_handlers_.find(message.id);
    // TODO: delete connections correctly
    assert(message_handlers_.end() != handler_it);
    auto& message_handler = handler_it->second;
    try {
        // Dispatch message
        auto action = Dispatcher()(message.raw_message, message_handler);

        // Change message
        auto new_message = Message(
            message.id, message.origin,
            ChangerDispatcher()(message.raw_message, message_changer_)
        );

        // Process message
        switch (action) {
        case Action::FORWARD:
            forward_message(new_message);
            break;
        case Action::ENQUEUE:
            enqueue_message(new_message);
            break;
        case Action::DROP:
            break;
        }
    }
    catch (const std::invalid_argument& error) {
        std::cerr << "Pipeline process error: " << error.what() << std::endl;

        // Fast-forward the message
        forward_message(message);
    }
}

void MessagePipeline::forward_message(Message message)
{
    sender_.send(message);
}

void MessagePipeline::enqueue_message(Message message)
{
    queue_.push(std::move(message));
}

} // namespace pipeline
