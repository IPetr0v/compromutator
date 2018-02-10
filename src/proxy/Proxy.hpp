#pragma once

#include "../ConcurrencyPrimitives.hpp"
#include "Event.hpp"
#include "ConnectionManager.hpp"

#include <memory>
#include <string>

class Proxy
{
public:
    Proxy(ProxySettings settings, std::shared_ptr<Alarm> alarm):
        event_queue_(alarm)
    {
        connection_manager_ = std::make_unique<ConnectionManager>(
            settings, event_queue_
        );
    }

    bool eventsExist() {
        return not event_queue_.empty();
    }

    EventPtr getEvent() {
        return event_queue_.pop();
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
