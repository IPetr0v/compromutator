#pragma once

#include "../ConcurrentQueue.hpp"

#include <fluid/base/BaseOFConnection.hh>

#include <memory>

using ConnectionId = uint32_t;

struct Message
{
    Message(uint8_t type, void* raw_data, size_t len):
        type(type), data(reinterpret_cast<uint8_t*>(raw_data)), len(len) {
    }

    // TODO: change to uint8_t*
    //auto deleter = [this](void* ptr){ free_data(ptr); };
    //std::unique_ptr<void, decltype(deleter)> data {data_, deleter};
    //using Deleter = decltype(fluid_base::BaseOFConnection::free_data);
    uint8_t type;
    uint8_t* data;
    size_t len;

private:
};

enum class Origin
{
    FROM_CONTROLLER,
    FROM_SWITCH
};

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
                          Origin origin, Message message):
        Event(EventType::MESSAGE), connection_id(connection_id),
        origin(origin), message(std::move(message)) {}

    ConnectionId connection_id;
    Origin origin;
    Message message;

    static MessageEventPtr pointerCast(EventPtr event) {
        return std::dynamic_pointer_cast<MessageEvent>(event);
    }
};

using EventQueue = ConcurrentQueue<EventPtr>;
