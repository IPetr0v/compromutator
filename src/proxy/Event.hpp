#pragma once

#include "../ConcurrencyPrimitives.hpp"
#include "../Types.hpp"

#include <fluid/base/BaseOFConnection.hh>

#include <memory>

enum class EventType
{
    CONNECTION,
    MESSAGE
};

enum class ConnectionEventType
{
    NEW,
    CLOSED
};

struct Event
{
    virtual ~Event() = default;

    EventType type;

protected:
    explicit Event(EventType type): type(type) {}
};
using EventPtr = std::shared_ptr<Event>;

struct ConnectionEvent;
using ConnectionEventPtr = std::shared_ptr<ConnectionEvent>;
struct ConnectionEvent : public Event
{
    explicit ConnectionEvent(ConnectionId id, ConnectionEventType type):
        Event(EventType::CONNECTION), id(id), type(type) {}

    ConnectionId id;
    ConnectionEventType type;

    static ConnectionEventPtr pointerCast(EventPtr event) {
        return std::dynamic_pointer_cast<ConnectionEvent>(event);
    }
};

struct MessageEvent;
using MessageEventPtr = std::shared_ptr<MessageEvent>;
struct MessageEvent : public Event
{
    explicit MessageEvent(ConnectionId connection_id,
                          Origin origin, RawMessage message):
        Event(EventType::MESSAGE),
        message(connection_id, origin, std::move(message)) {}

    Message message;

    static MessageEventPtr pointerCast(EventPtr event) {
        return std::dynamic_pointer_cast<MessageEvent>(event);
    }
};

using EventQueue = ConcurrentAlarmingQueue<EventPtr>;
