#pragma once

#include "Event.hpp"
#include "ConnectionManager.hpp"

#include <memory>
#include <string>

class Proxy
{
public:
    Proxy(ProxySettings settings) {
        connection_manager_ = std::make_unique<ConnectionManager>(
            settings, event_queue_
        );
    }

    EventPtr getEvent(std::chrono::milliseconds timeout_duration) {
        return event_queue_.pop(timeout_duration);
    }

    void sendToController(ConnectionId id, Message message) {
        connection_manager_->sendToController(id, message);
    }

    void sendToSwitch(ConnectionId id, Message message) {
        connection_manager_->sendToSwitch(id, message);
    }

private:
    EventQueue event_queue_;
    // TODO: consider changing ConnectionManager to ProxyImpl
    std::unique_ptr<ConnectionManager> connection_manager_;

};
