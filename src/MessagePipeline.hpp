#pragma once

#include "openflow/Parser.hpp"
#include "Types.hpp"

#include <queue>

namespace pipeline {

enum class Action {FORWARD, ENQUEUE, DROP};

using Dispatcher = MessageDispatcher<Action, fluid_msg::OFMsg>;
template<class Message> using Visitor = Dispatcher::Visitor<Message>;

using ChangerDispatcher = MessageDispatcher<RawMessage, fluid_msg::OFMsg>;
template<class Message> using Changer = ChangerDispatcher::Visitor<Message>;

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
        connection_id_(id), controller_(controller), is_established_(false) {}

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
    Controller& controller_;

    bool is_established_;
    SwitchInfoStatus info_status_;
    SwitchInfo info_;

    void try_establish();
};

class MessageHandler : public Visitor<fluid_msg::of13::FlowMod>,
                       public Visitor<fluid_msg::of13::MultipartReplyFlow>,
                       public Visitor<fluid_msg::OFMsg>
{
public:
    MessageHandler(ConnectionId id, Controller& controller, Detector& detector):
        connection_id_(id), controller_(controller), detector_(detector) {}

    // Controller messages
    Action visit(fluid_msg::of13::FlowMod&) override;

    // Switch messages
    Action visit(fluid_msg::of13::MultipartReplyFlow&) override;

    // Default
    Action visit(fluid_msg::OFMsg&) override;

private:
    ConnectionId connection_id_;
    Controller& controller_;
    Detector& detector_;

    RuleStatsFields get_rule_stats(const FlowStatsCommon& flow_stats) const;
    //PortStatsFields get_port_stats();
};

struct MessageChanger : public Changer<fluid_msg::of13::FlowMod>,
                        public Changer<fluid_msg::of13::MultipartRequestFlow>,
                        public Changer<fluid_msg::of13::MultipartReplyFlow>,
                        public Changer<fluid_msg::OFMsg>
{
    // Controller messages
    RawMessage visit(fluid_msg::of13::FlowMod& flow_mod) override;
    RawMessage visit(fluid_msg::of13::MultipartRequestFlow&) override;

    // Switch messages
    RawMessage visit(fluid_msg::of13::MultipartReplyFlow&) override;

    // Default
    RawMessage visit(fluid_msg::OFMsg&) override;
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
    void flushPipeline();

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
