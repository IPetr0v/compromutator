#pragma once

#include "Types.hpp"
#include "../NetworkSpace.hpp"

#include <fluid/ofcommon/msg.hh>
#include <fluid/of10/openflow-10.h>
#include <fluid/of13/openflow-13.h>
#include <fluid/of13msg.hh>

#include <bitset>
#include <utility>

// This class is used to check whether Container has mask() function
template<typename Container>
struct IsMasked
{
    template <typename C>
    static auto is_masked(decltype(&C::mask)*) -> std::true_type;

    template<typename C>
    static auto is_masked(...) -> std::false_type;

    using type = decltype(is_masked<Container>(nullptr));
    static const int value = type::value;
};

class Mapping
{
private:
    struct Begin {
        constexpr static uint32_t OFFSET = 0u;
        constexpr static uint32_t SIZE = 0u;
    };

    template<class Container, uint32_t Size, class Previous = Begin>
    struct Base {
        constexpr static uint32_t OFFSET = Previous::OFFSET + Previous::SIZE;
        constexpr static uint32_t SIZE = Size;

        // TODO: header space size should be Size / 8
        using BitSet = std::bitset<Size>;
        struct MaskedBitSet {
            explicit MaskedBitSet(BitSet value):
                value(value), mask(BitSet().set()) {}
            MaskedBitSet(BitSet value, BitSet mask):
                value(value), mask(mask) {}

            BitSet value;
            BitSet mask;
        };

        using FieldType = Container;
        using ValueType = typename std::decay<
            decltype(std::declval<Container>().value())
        >::type;

        // Masked
        template<
            class Type,
            typename std::enable_if_t<IsMasked<Type>::value>* = nullptr
        >
        static MaskedBitSet toBitSet(const Type& object) {
            auto value = get_value(object->value());
            auto mask = get_value(object->mask());
            return MaskedBitSet(value, mask);
        }

        // Unmasked
        template<
            class Type,
            typename std::enable_if_t<not IsMasked<Type>::value>* = nullptr
        >
        static MaskedBitSet toBitSet(const Type& object) {
            auto value = get_value(object->value());
            return MaskedBitSet(value);
        }

        //static FieldType* fromBitSet(MaskedBitSet bitset);

    private:
        static uint64_t get_value(const ValueType&);

    };

public:
    // L2
    using EthSrc = Base<fluid_msg::of13::EthSrc, 48u>;
    using EthDst = Base<fluid_msg::of13::EthDst, 48u, EthSrc>;
    using EthType = Base<fluid_msg::of13::EthType, 1u, EthDst>;

    // L3
    using IPProto = Base<fluid_msg::of13::IPProto, 1u, EthType>;
    using IPv4Src = Base<fluid_msg::of13::IPv4Src, 32u, IPProto>;
    using IPv4Dst = Base<fluid_msg::of13::IPv4Dst, 32u, IPv4Src>;

    // L4
    using TCPSrc = Base<fluid_msg::of13::TCPSrc, 16u, IPv4Dst>;
    using TCPDst = Base<fluid_msg::of13::TCPDst, 16u, TCPSrc>;
    using UDPSrc = Base<fluid_msg::of13::UDPSrc, 16u, IPv4Dst>;
    using UDPDst = Base<fluid_msg::of13::UDPDst, 16u, UDPSrc>;

};

class BitVectorBridge
{
public:
    explicit BitVectorBridge(BitVector&& bit_vector):
        bit_vector_(std::move(bit_vector)) {}

    BitVector copyBitVector() const {
        return bit_vector_;
    }
    BitVector popBitVector() {
        return std::move(bit_vector_);
    }

    template<class Map>
    void setField(typename Map::FieldType* fluid_object) {
        auto bitset = Map::toBitSet(fluid_object);
        set_bits<Map>(bitset);
    }

private:
    BitVector bit_vector_;

    template<class Map>
    typename Map::MaskedBitSet get_bits() const {
        auto value = typename Map::BitSet();
        auto mask = typename Map::BitSet();
        for (uint32_t index = 0; index < Map::SIZE; index++) {
            auto bit_value = bit_vector_.getBit(Map::OFFSET + index);
            switch (bit_value) {
            case BitValue::ZERO:
                value[index] = false;
                mask[index] = true;
                break;
            case BitValue::ONE:
                value[index] = true;
                mask[index] = true;
                break;
            case BitValue::ANY:
                value[index] = false;
                mask[index] = false;
                break;
            case BitValue::NONE:
            default:
                return nullptr;
            }
        }
        return {std::move(value), std::move(mask)};
    }

    template<class Map>
    void set_bits(typename Map::MaskedBitSet bitset) {
        for (uint32_t index = 0; index < Map::SIZE; index++) {
            if (true == bitset.mask[index]) {
                auto bit_value = (true == bitset.value[index])
                                 ? BitValue::ONE
                                 : BitValue::ZERO;
                bit_vector_.setBit(Map::OFFSET + index, bit_value);
            }
        }
    }


    /*using ValueType = typename std::result_of<
        decltype(std::declval<Container>().value())
    >::type;*/
    //    decltype(&Container::value)(Container)
    //    typename std::decay<decltype(std::declval<Container>().value())>::type



    /*template<class Map>
    void set_bits(typename Map::BitSet value) {
        for (uint32_t index = 0; index < Map::SIZE; index++) {
            auto bit_value = (true == value[index]) ? BitValue::ONE
                                                    : BitValue::ZERO;
            bit_vector_.setBit(Map::OFFSET + index, bit_value);
        }
    }*/




    /*of13::OXMTLV getField(uint8_t type) const;

    of13::EthSrc* getEthSrc() const;
    void setEthSrc(of13::EthSrc* eth_src);

    of13::EthDst* getEthDst() const;
    void setEthDst(of13::EthDst* eth_dst);

    void setField(of13::EthType* eth_type);
    void setField(of13::IPProto* ip_proto);
    void setField(of13::IPv4Src* ipv4_src);
    void setField(of13::IPv4Dst* ipv4_dst);
    void setField(of13::TCPSrc* tcp_src);
    void setField(of13::TCPDst* tcp_dst);
    void setField(of13::UDPSrc* udp_src);
    void setField(of13::UDPDst* udp_dst);*/




    /*template<class BitSetType>
    struct MaskedValue {BitSetType value; BitSetType mask;};

    template<class BitSetType>
    MaskedValue<BitSetType> get_masked_value(uint32_t offset, uint32_t size);
    template<class BitSetType>
    void set_masked_value(uint32_t offset, uint32_t size,
                          MaskedValue<BitSetType>&& masked_value);

    static_assert(6 == Size::ETH_SRC/8, "Proto mapping error");
    static_assert(Size::ETH_SRC == Size::ETH_DST, "Proto mapping error");
    using EthAddrBitSet = std::bitset<Size::ETH_SRC/8>;

    static_assert(4 == Size::IPV4_SRC/8, "Proto mapping error");
    static_assert(Size::IPV4_SRC == Size::IPV4_DST, "Proto mapping error");
    using IPv4AddrBitSet = std::bitset<Size::IPV4_SRC/8>;

    EthAddrBitSet to_bitset(const EthAddress& eth_address) const;
    EthAddress get_eth_address(const EthAddrBitSet& bitset) const;

    IPv4AddrBitSet to_bitset(const IPAddress& ipv4_address) const;
    IPAddress get_ip_address(const IPv4AddrBitSet& bitset) const;*/

};
