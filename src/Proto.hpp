#pragma once

#include <fluid/util/util.h>

#include <cstdint>
#include <cstring>
#include <arpa/inet.h>
#include <iterator>
#include <memory>
#include <sstream>
#include <vector>

namespace proto {

struct __attribute__((packed)) Ethernet
{
    uint8_t dst[6];
    uint8_t src[6];
    uint16_t type;

    enum TYPE : uint16_t {
        IPv4  = 0x0800,
        ARP   = 0x0806,
        DOT1Q = 0x8100,
        IPv6  = 0x86dd,
        LLDP  = 0x88cc
    };
};
static_assert(sizeof(Ethernet) == 14, "");

struct __attribute__((packed)) Dot1Q
{
    uint8_t pcp:3;
    bool dei:1;
    uint16_t vid:12;
    uint16_t type;
};
static_assert(sizeof(Dot1Q) == 4, "");

struct __attribute__((packed)) IPv4
{
    uint8_t version:4;
    uint8_t ihl:4;
    uint8_t dscp:6;
    uint8_t ecn:2;
    uint16_t total_len;
    uint16_t identification;
    uint16_t flags:3;
    uint16_t fragment_offset_unordered:13;
    uint8_t ttl;
    uint8_t proto;
    uint16_t checksum;
    uint32_t src;
    uint32_t dst;

    enum PROTO : uint8_t {
        ICMP = 0x01,
        TCP  = 0x06,
        UDP  = 0x11
    };
};
static_assert(sizeof(IPv4) == 20, "");

class LLDP
{
public:
    struct Tlv {
        explicit Tlv(const uint8_t* data):
            value(nullptr)
        {
            auto header = reinterpret_cast<const uint16_t*>(data);
            type = ntoh16(*header) >> 9;
            length = ntoh16(*header) & 0x01FF;
            if (length > 0) {
                value = new uint8_t[length];
                std::memcpy(value, data + 2, length);
            }
        }
        Tlv(const Tlv& other):
            type(other.type), length(other.length), value(nullptr) {
            if (length > 0) {
                value = new uint8_t[length];
                std::memcpy(value, other.value, length);
            }
        }
        ~Tlv() {delete value;}

        size_t size() const {
            return 2 * sizeof(uint8_t) + length;
        }

        uint8_t type;
        size_t length;
        uint8_t* value;

        enum TYPE : uint8_t {
            END                 = 0,
            CHASSIS_ID          = 1,
            PORT_ID             = 2,
            TTL                 = 3,
            PORT_DESCRIPTION    = 4,
            SYSTEM_NAME         = 5,
            SYSTEM_DESCRIPTION  = 6,
            SYSTEM_CAPABILITIES = 7,
            MANAGEMENT_ADDRESS  = 8,
            CUSTOM              = 127
        };

        struct Id {
            Id(uint8_t tlv_length, const uint8_t* tlv_value):
                type(*tlv_value),
                value(tlv_value + 1u, tlv_value + tlv_length) {}

            uint8_t type;
            std::vector<uint8_t> value;

            std::string stringValue() const {
                std::ostringstream oss;
                std::copy(value.begin(), value.end(),
                          std::ostream_iterator<uint8_t>(oss));
                return oss.str();
            }

            uint64_t decimalValue() const {
                uint64_t result = 0u;
                for (auto digit : value) {
                    result *= 10u;
                    result += digit;
                }
                return result;
            }
        };

        struct ChassisId: public Id {
            ChassisId(uint8_t tlv_length, const uint8_t* tlv_value):
                Id(tlv_length, tlv_value) {}

            enum TYPE : uint8_t {
                MAC   = 4,
                LOCAL = 7
            };
        };

        struct PortId: public Id {
            PortId(uint8_t tlv_length, const uint8_t* tlv_value):
                Id(tlv_length, tlv_value) {}

            enum TYPE : uint8_t {
                ALIAS     = 1,
                COMPONENT = 2,
                MAC       = 3,
                ADDRESS   = 4,
                NAME      = 5,
                CIRCUIT   = 6,
                LOCAL     = 7
            };
        };
    };

    LLDP(void* raw_data, size_t length) {
        auto data = reinterpret_cast<uint8_t*>(raw_data);
        if (sizeof(Ethernet) <= length) {
            auto eth = reinterpret_cast<Ethernet*>(data);
            if (Ethernet::TYPE::LLDP == ntoh16(eth->type)) {
                init_tlv_list(data + sizeof(Ethernet),
                              length - sizeof(Ethernet));
            }
            else if (Ethernet::TYPE::DOT1Q == ntoh16(eth->type)) {
                // TODO: add QinQ
                auto dot1q = reinterpret_cast<Dot1Q*>(data + sizeof(Ethernet));
                if (Ethernet::TYPE::LLDP == ntoh16(dot1q->type)) {
                    init_tlv_list(data + sizeof(Ethernet) + sizeof(Dot1Q),
                                  length - sizeof(Ethernet) - sizeof(Dot1Q));
                }
            }
            else {
                throw std::invalid_argument("Not an LLDP packet");
            }
        }
        else {
            throw std::invalid_argument("Too small ethernet frame");
        }

        if (not is_correct()) {
            throw std::invalid_argument("Incorrect LLDP");
        }
    }

    std::shared_ptr<Tlv::ChassisId> chassis_id;
    std::shared_ptr<Tlv::PortId> port_id;

private:
    std::vector<Tlv> tlv_list_;
    bool ttl_present_, end_present_;

    bool is_correct() const {
        return chassis_id && port_id && ttl_present_ && end_present_;
    };

    void init_tlv_list(uint8_t* data, size_t length) {
        size_t offset = 0;
        while (offset < length) {
            tlv_list_.emplace_back(data + offset);
            init_specific_tlv(tlv_list_.back());
            offset += tlv_list_.back().size();
        }
    }

    void init_specific_tlv(const Tlv& tlv) {
        switch (tlv.type) {
        case Tlv::TYPE::CHASSIS_ID:
            chassis_id = std::make_unique<Tlv::ChassisId>(tlv.length, tlv.value);
            break;
        case Tlv::TYPE::PORT_ID:
            port_id = std::make_unique<Tlv::PortId>(tlv.length, tlv.value);
            break;
        case Tlv::TYPE::TTL:
            ttl_present_ = true;
            break;
        case Tlv::TYPE::END:
            end_present_ = true;
            break;
        default: break;
        }
    }
};

} // namespace proto
