#pragma once

#include "Types.hpp"
#include "Mapping.hpp"
#include "../NetworkSpace.hpp"
#include "../Detector.hpp"
#include "../proxy/Proxy.hpp"

#include <fluid/ofcommon/msg.hh>
#include <fluid/of10/openflow-10.h>
#include <fluid/of13/openflow-13.h>
#include <fluid/of13msg.hh>

#include <bitset>

using namespace fluid_msg;

/*struct Mapping
{
    template<uint32_t Id, uint32_t Size, uint32_t Previous = 0>
    struct Mapping {
        constexpr static unsigned int PREVIOUS;
        constexpr static uint32_t OFFSET = 48;
        constexpr static uint32_t SIZE = 48;
    };

    struct EthBase {
        static_assert(6 == Size::ETH_SRC/8, "Proto mapping error");
        static_assert(Size::ETH_SRC == Size::ETH_DST, "Proto mapping error");
        using BitSet = std::bitset<Size::ETH_SRC/8>;
        static BitSet toBitSet(const EthAddress& eth_address);
    };

    struct EthSrc : public EthBase {
        constexpr static uint32_t OFFSET = 48;
        constexpr static uint32_t SIZE = 48;
        using BitSet = std::bitset<SIZE/8>;
    };

private:
    template<typename Offset, typename Size>
    struct Base {

    };

};

struct Size
{
    constexpr static uint32_t ETH_SRC = 48;
    constexpr static uint32_t ETH_DST = 48;
    constexpr static uint32_t ETH_TYPE = 1;
    constexpr static uint32_t L2 = ETH_SRC + ETH_DST + ETH_TYPE;

    constexpr static uint32_t IP_PROTO = 1;
    constexpr static uint32_t IPV4_SRC = 32;
    constexpr static uint32_t IPV4_DST = 32;
    constexpr static uint32_t L3 = IPV4_SRC + IPV4_DST + IP_PROTO;

    constexpr static uint32_t TCP_SRC = 16;
    constexpr static uint32_t TCP_DST = 16;
    constexpr static uint32_t UDP_SRC = 16;
    constexpr static uint32_t UDP_DST = 16;
    constexpr static uint32_t L4 = TCP_SRC + TCP_DST;
};

struct Offset
{
    constexpr static uint32_t L2 = 0;
    constexpr static uint32_t ETH_SRC = L2;
    constexpr static uint32_t ETH_DST = L2 + Size::ETH_SRC;
    constexpr static uint32_t ETH_TYPE = L2 + Size::ETH_SRC + Size::ETH_DST;

    constexpr static uint32_t L3 = Size::L2;
    constexpr static uint32_t IP_PROTO = L3;
    constexpr static uint32_t IPV4_SRC = L3 + Size::IP_PROTO;
    constexpr static uint32_t IPV4_DST = L3 + Size::IP_PROTO + Size::IPV4_SRC;

    constexpr static uint32_t L4 = Size::L3;
    constexpr static uint32_t TCP_SRC = L4;
    constexpr static uint32_t TCP_DST = L4 + Size::TCP_SRC;
    constexpr static uint32_t UDP_SRC = L4;
    constexpr static uint32_t UDP_DST = L4 + Size::UDP_SRC;
};

constexpr uint32_t OPENFLOW_HEADER_LENGTH = Size::L2 + Size::L3 + Size::L4;
*/

/*class MappingBegin {
    constexpr static uint32_t OFFSET = 0u;
    constexpr static uint32_t SIZE = 0u;
};

template<uint8_t Id, uint32_t Size, class Previous = MappingBegin>
struct Mapping {
    constexpr static uint32_t OFFSET = Previous::OFFSET + Previous::SIZE;
    constexpr static uint32_t SIZE = Size;
    using BitSet = std::bitset<Size/8>;
};

// L2
using EthSrc  = Mapping<of13::OFPXMT_OFB_ETH_SRC, 48u>;
using EthDst  = Mapping<of13::OFPXMT_OFB_ETH_DST, 48u, EthSrc>;
using EthType = Mapping<of13::OFPXMT_OFB_ETH_TYPE, 1u, EthDst>;

// L3
using IPProto = Mapping<of13::OFPXMT_OFB_IP_PROTO, 1u, EthType>;
using IPv4Src = Mapping<of13::OFPXMT_OFB_IPV4_SRC, 32u, IPProto>;
using IPv4Dst = Mapping<of13::OFPXMT_OFB_IPV4_DST, 32u, IPv4Src>;

// L4
using TCPSrc = Mapping<of13::OFPXMT_OFB_TCP_SRC, 16u, IPv4Dst>;
using TCPDst = Mapping<of13::OFPXMT_OFB_TCP_DST, 16u, TCPSrc>;
using UDPSrc = Mapping<of13::OFPXMT_OFB_UDP_SRC, 16u, IPv4Dst>;
using UDPDst = Mapping<of13::OFPXMT_OFB_UDP_DST, 16u, UDPSrc>;*/

class Parser
{
public:
    RuleInfo getRuleInfo(of13::FlowMod& flow_mod) const {
        TableId table_id = flow_mod.table_id();
        Priority priority = flow_mod.priority();
        NetworkSpace domain = get_domain(flow_mod.match());
        ActionsBase actions = get_actions(flow_mod.instructions());

        return {table_id, priority, std::move(domain), std::move(actions)};
    }

    of13::FlowMod getFlowMod(RulePtr rule) const {
        of13::FlowMod flow_mod;
        return std::move(flow_mod);
    }

private:
    // TODO: rewrite fluid_msg so we do not need to copy match (add const)
    NetworkSpace get_domain(of13::Match match) const;
    of13::Match get_match(const NetworkSpace& domain) const;

    ActionsBase get_actions(of13::InstructionSet instructions) const;
    ActionsBase get_apply_actions(of13::ApplyActions* apply_actions) const;
    of13::InstructionSet get_instructions(Actions actions) const;
};
