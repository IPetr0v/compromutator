#pragma once

#include <fluid/ofcommon/msg.hh>

#include <cstdint>
#include <iostream>
#include <memory>
#include <tuple>
#include <queue>
#include <vector>

using ConnectionId = uint32_t;

using SwitchId = uint64_t;
using PortId = uint32_t;
using TableId = uint8_t;
using GroupId = uint32_t;
using TopoId = std::pair<SwitchId, PortId>;

using Priority = uint16_t;
constexpr Priority ZERO_PRIORITY = 0u;
constexpr Priority LOW_PRIORITY = 1u;

using Cookie = uint64_t;
constexpr Cookie ZERO_COOKIE = 0x0;

using RuleId = std::tuple<SwitchId, TableId, Priority, uint64_t>;

template <typename IdType>
class IdGenerator
{
public:
    IdGenerator(): last_id_((IdType)1) {}

    IdType getId() {return last_id_++;}
    void releaseId(IdType id) {/* TODO: implement release id */}

private:
    IdType last_id_;

};

using RequestId = uint32_t;
using RequestIdGenerator = IdGenerator<RequestId>;

struct RuleStatsFields
{
    uint64_t packet_count; /* Number of packets matched by a flow entry. */
    uint64_t byte_count;   /* Number of bytes matched by a flow entry. */
};

struct PortStatsFields
{
    uint64_t rx_packets;   /* Number of received packets. */
    uint64_t tx_packets;   /* Number of transmitted packets. */
    uint64_t rx_bytes;     /* Number of received bytes. */
    uint64_t tx_bytes;     /* Number of transmitted bytes. */
    uint64_t rx_dropped;   /* Number of packets dropped by RX. */
    uint64_t tx_dropped;   /* Number of packets dropped by TX. */
    uint64_t rx_errors;    /* Number of receive errors. This is a super-set
                              of receive errors and should be great than or
                              equal to the sum of al rx_*_err values. */
    uint64_t tx_errors;    /* Number of transmit errors. This is a super-set
                              of transmit errors. */
    uint64_t rx_frame_err; /* Number of frame alignment errors. */
    uint64_t rx_over_err;  /* Number of packets with RX overrun. */
    uint64_t rx_crc_err;   /* Number of CRC errors. */
    uint64_t collisions;   /* Number of collisions. */
};

struct RawMessage
{
    RawMessage(uint8_t type, void* raw_data, size_t len):
        type(type), len(len), data_(reinterpret_cast<uint8_t*>(raw_data)) {}
    RawMessage(fluid_msg::OFMsg& message):
        RawMessage(message.type(), message.pack(), message.length())
    {
        // Workaround for the libfluid multipart.pack - it changes message len!
        auto fixed_len = message.length();
        len = (fixed_len > len) ? fixed_len : len;
    }

    uint8_t type;
    size_t len;
    uint8_t* data() const {return data_.get();}

private:
    std::shared_ptr<uint8_t> data_;
};

enum class Origin
{
    FROM_CONTROLLER,
    FROM_SWITCH
};

enum class Destination
{
    TO_CONTROLLER,
    TO_SWITCH
};

struct Message {
    Message(ConnectionId connection_id, Origin origin,
            Destination destination, RawMessage raw_message):
        connection_id(connection_id), origin(origin), destination(destination),
        raw_message(raw_message) {}

    Message(ConnectionId connection_id, Origin origin,
            RawMessage raw_message):
        Message(connection_id, origin, get_destination(origin), raw_message) {}
    Message(ConnectionId connection_id, Destination destination,
            RawMessage raw_message):
        Message(connection_id, get_origin(destination),
                destination, raw_message) {}

    ConnectionId connection_id;
    Origin origin;
    Destination destination;
    RawMessage raw_message;

private:
    Origin get_origin(Destination destination) {
        return destination == Destination::TO_SWITCH
               ? Origin::FROM_CONTROLLER
               : Origin::FROM_SWITCH;
    }
    Destination get_destination(Origin origin) {
        return origin == Origin::FROM_CONTROLLER
               ? Destination::TO_SWITCH
               : Destination::TO_CONTROLLER;
    }
};

// TODO: move to primitives
template<class Element>
class QueueWithBarriers
{
public:
    bool empty() const {
        return queue_.empty();
    }

    size_t size() const {
        return queue_.size();
    }

    void addBarrier() {
        queue_.push(std::vector<Element>());
    }

    void push(Element&& element) {
        if (queue_.empty()) {
            queue_.push(std::vector<Element>{std::move(element)});
        }
        else {
            queue_.back().push_back(std::move(element));
        }
    }

    std::vector<Element> pop() {
        auto element = std::move(queue_.front());
        queue_.pop();
        return std::move(element);
    }

private:
    std::queue<std::vector<Element>> queue_;
};
