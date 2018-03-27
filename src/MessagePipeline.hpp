#pragma once

#include "openflow/Parser.hpp"
#include "Types.hpp"

#include <queue>

namespace pipeline {

enum class Action {FORWARD, ENQUEUE, DROP};

using Dispatcher = MessageDispatcher<Action>;
using VisitorInterface = Dispatcher::VisitorInterface;

using ChangerDispatcher = MessageDispatcher<RawMessage>;
using ChangerInterface = ChangerDispatcher::VisitorInterface;

class HandshakeHandler : public VisitorInterface
{
    struct SwitchInfoStatus {
        SwitchInfoStatus(): features(false), ports(false) {}
        bool features, ports;
        bool isFull() const {return features and ports;}
    };

public:
    HandshakeHandler(ConnectionId id, Controller& controller):
        connection_id_(id), controller_(controller), is_established_(false) {}

    bool established() const {return is_established_;}

    Action visit(fluid_msg::of13::Hello& hello);
    Action visit(fluid_msg::of13::FeaturesReply& features_reply);
    Action visit(fluid_msg::of13::MultipartReplyPortDescription& port_desc);
    Action visit(fluid_msg::OFMsg& message);

private:
    ConnectionId connection_id_;
    Controller& controller_;

    bool is_established_;
    SwitchInfoStatus info_status_;
    SwitchInfo info_;

    Action try_establish();
};

class MessageHandler : public VisitorInterface
{
public:
    MessageHandler(ConnectionId id, Controller& controller, Detector& detector):
        connection_id_(id), controller_(controller), detector_(detector) {}

    // Controller messages
    Action visit(fluid_msg::of13::FlowMod& flow_mod);

    // Switch messages
    Action visit(fluid_msg::of13::MultipartReplyFlow& reply_flow);

    // Other messages
    Action visit(fluid_msg::OFMsg& message);

private:
    ConnectionId connection_id_;
    Controller& controller_;
    Detector& detector_;

    RuleStatsFields get_rule_stats(const FlowStatsCommon& flow_stats) const;
    //PortStatsFields get_port_stats();
};

struct MessageChanger : public ChangerInterface
{
    // Controller messages
    RawMessage visit(fluid_msg::of13::FlowMod& flow_mod);
    RawMessage visit(fluid_msg::of13::MultipartRequestFlow& request_flow);

    // Switch messages
    RawMessage visit(fluid_msg::of13::MultipartReplyFlow& reply_flow);

    // Other messages
    //RawMessage visit(fluid_msg::OFMsg& message);
};

class MessagePipeline
{
    struct HandshakePipeline {
        HandshakePipeline(ConnectionId id, Controller& controller):
            handler(id, controller) {}

        HandshakeHandler handler;
        std::queue<Message> queue;
    };

public:
    MessagePipeline(Sender sender, Controller& controller, Detector& detector):
        sender_(sender), controller_(controller), detector_(detector) {}

    void addConnection(ConnectionId id);
    void deleteConnection(ConnectionId id);
    void processMessage(Message message);

    void addBarrier();
    void flushPipeline(ConnectionId id);

private:
    Sender sender_;
    Controller& controller_;
    Detector& detector_;

    std::unordered_map<ConnectionId, HandshakePipeline> handshake_pipelines_;
    std::unordered_map<ConnectionId, MessageHandler> message_handlers_;
    MessageChanger message_changer_;

    QueueWithBarriers<Message> queue_;

    void handle_message(Message message);
    void forward_message(Message message);
    void enqueue_message(Message message);
};

} // namespace pipeline
