#pragma once

#include "../ConcurrencyPrimitives.hpp"
#include "Event.hpp"
#include "ConnectionManager.hpp"

#include <memory>
#include <string>

class Sender
{
public:
    explicit Sender(std::weak_ptr<ConnectionManager> connection_manager):
        connection_manager_(connection_manager) {}

    void send(ConnectionId id, Destination destination, RawMessage message) {
        if (auto manager = connection_manager_.lock()) {
            if (destination == Destination::TO_CONTROLLER) {
                manager->sendToController(id, message);
            }
            else {
                manager->sendToSwitch(id, message);
            }
        }
        else {
            std::cerr << "Sender error: No connection manager" << std::endl;
        }
    }

    void send(Message message) {
        send(message.id, message.destination, message.raw_message);
    }

private:
    std::weak_ptr<ConnectionManager> connection_manager_;
};

class Proxy
{
public:
    Proxy(ProxySettings settings, std::shared_ptr<Alarm> alarm):
        event_queue_(alarm)
    {
        connection_manager_ = std::make_shared<ConnectionManager>(
            settings, event_queue_
        );
    }

    // TODO: is this const?
    bool eventsExist() {
        return not event_queue_.empty();
    }

    EventPtr getEvent() {
        return event_queue_.pop();
    }

    Sender getSender() const {
        return Sender(connection_manager_);
    }

    /*void sendToController(ConnectionId id, RawMessage message) {
        connection_manager_->sendToController(id, message);
    }

    void sendToSwitch(ConnectionId id, RawMessage message) {
        connection_manager_->sendToSwitch(id, message);
    }

    void send(ConnectionId id, Destination destination, RawMessage message) {
        if (destination == Destination::TO_CONTROLLER) {
            connection_manager_->sendToController(id, message);
        }
        else {
            connection_manager_->sendToSwitch(id, message);
        }
    }*/

private:
    EventQueue event_queue_;
    std::shared_ptr<ConnectionManager> connection_manager_;

};
