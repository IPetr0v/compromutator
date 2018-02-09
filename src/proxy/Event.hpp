#pragma once

#include <cassert>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

using ConnectionId = uint32_t;

struct Message
{
    Message(uint8_t type, void* data, size_t len):
        type(type), data(data), len(len) {}

    uint8_t type;
    void* data;
    size_t len;
};

enum class Origin
{
    FROM_CONTROLLER,
    FROM_SWITCH
};

enum class EventType
{
    TIMEOUT,
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

struct TimeoutEvent : public Event
{
    TimeoutEvent(): Event(EventType::TIMEOUT) {}
};

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

class EventQueue
{
public:
    void push(EventPtr&& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.push(std::move(event));
        is_available_.notify_one();
    }

    EventPtr pop(std::chrono::milliseconds timeout_duration) {
        std::unique_lock<std::mutex> wait_lock(mutex_);
        if (events_.empty()) {
            auto status = is_available_.wait_for(wait_lock, timeout_duration);
            if (std::cv_status::no_timeout == status) {
                return this->pop();
            } else {
                return std::make_unique<TimeoutEvent>();
            }
        }
        else {
            return this->pop();
        }
    }

private:
    std::mutex mutex_;
    std::condition_variable is_available_;
    std::queue<EventPtr> events_;

    EventPtr pop() {
        auto event = std::move(events_.front());
        events_.pop();
        return std::move(event);
    }
};
