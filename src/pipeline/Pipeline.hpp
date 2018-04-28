#pragma once

#include "HandshakeHandler.hpp"
#include "MessageHandler.hpp"
#include "MessageChanger.hpp"
#include "../Types.hpp"
#include "../Controller.hpp"

#include <queue>

namespace pipeline {

class Pipeline
{
    struct HandshakePipeline {
        HandshakePipeline(ConnectionId id, Controller& controller):
            handler(id, controller) {}

        HandshakeHandler handler;
        std::queue<Message> queue;
    };

public:
    Pipeline(Sender sender, Controller& controller):
        sender_(sender), controller_(controller) {}

    void addConnection(ConnectionId id);
    void deleteConnection(ConnectionId id);
    void processMessage(Message message);

    void addBarrier();
    void flushPipeline();

    size_t queueSize() const {return queue_.size();}

private:
    Sender sender_;
    Controller& controller_;

    std::unordered_map<ConnectionId, HandshakePipeline> handshake_pipelines_;
    std::unordered_map<ConnectionId, MessageHandler> message_handlers_;
    MessageChanger message_changer_;

    QueueWithBarriers<Message> queue_;

    void handle_message(Message message);
    void forward_message(Message message);
    void enqueue_message(Message message);
};

} // namespace pipeline
