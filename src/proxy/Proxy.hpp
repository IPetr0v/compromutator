#pragma once

#include "../ConcurrencyPrimitives.hpp"
#include "Event.hpp"
#include "ConnectionManager.hpp"

#include <fluid/ofcommon/msg.hh>

#include <memory>
#include <string>

class Sender
{
public:
    explicit Sender(std::weak_ptr<ConnectionManager> connection_manager):
        connection_manager_(connection_manager) {}

    // TODO: add const to add pack functions in fluid_msg
    void send(ConnectionId id, Destination destination,
              fluid_msg::OFMsg& message) {
        RawMessage raw_message(message);
        send(id, destination, std::move(raw_message));
    }

    void send(ConnectionId id, Destination destination, RawMessage message) {
        if (auto manager = connection_manager_.lock()) {
            if (destination == Destination::TO_CONTROLLER) {
                //std::cout<<"Send to controller"<<std::endl;
                manager->sendToController(id, message);
            }
            else {
                //std::cout<<"Send to switch"<<std::endl;
                manager->sendToSwitch(id, message);
            }
        }
        else {
            std::cerr << "Sender error: No connection manager" << std::endl;
        }
    }

    void send(Message message) {
        send(message.connection_id, message.destination, message.raw_message);
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

    bool eventsExist() {
        return not event_queue_.empty();
    }

    EventPtr getEvent() {
        return event_queue_.pop();
    }

    Sender getSender() const {
        return Sender(connection_manager_);
    }

private:
    EventQueue event_queue_;
    std::shared_ptr<ConnectionManager> connection_manager_;

};
