#pragma once

#include <cstdint>
#include <iostream>
#include <tuple>
#include <vector>

using SwitchId = uint64_t;
using PortId = uint32_t;
using TableId = uint8_t;
using GroupId = uint32_t;
using TopoId = std::pair<SwitchId, PortId>;

using Priority = uint16_t;
constexpr Priority LOW_PRIORITY = 0;
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

using RequestId = uint64_t;
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
