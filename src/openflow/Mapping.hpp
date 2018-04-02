#pragma once

#include "../Types.hpp"
#include "../NetworkSpace.hpp"

#include <fluid/ofcommon/msg.hh>
#include <fluid/of10/openflow-10.h>
#include <fluid/of13/openflow-13.h>
#include <fluid/of13msg.hh>

#include <bitset>
#include <stdexcept>
#include <utility>

// This class is used to check if the Container has a mask() function
template<typename Container>
struct IsMasked
{
    // Using declval because fluid messages have 2 mask functions (setter and getter)
    template <typename C>
    static auto is_masked(
        std::decay<decltype(std::declval<C>().mask())>*
    ) -> std::true_type;

    template<typename C>
    static auto is_masked(...) -> std::false_type;

    using type = decltype(is_masked<Container>(nullptr));
    static const bool value = type::value;
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

        static MaskedBitSet toBitSet(const FieldType* object) {
            return std::move(to_bitset<FieldType>(object));
        }

        // TODO: change fluid behaviour - return shared_ptr
        static FieldType* fromBitSet(MaskedBitSet&& bitset) {
            return from_bitset<FieldType>(std::move(bitset));
        }

    private:
        // Masked
        template<
            class Type,
            typename std::enable_if_t<IsMasked<Type>::value>* = nullptr
        >
        static MaskedBitSet to_bitset(const Type* object) {
            auto value = get_integer_value(object->value());
            auto mask = get_integer_value(object->mask());
            return MaskedBitSet(value, mask);
        }

        template<
            class Type,
            typename std::enable_if_t<IsMasked<Type>::value>* = nullptr
        >
        static Type* from_bitset(MaskedBitSet&& bitset) {
            auto value = get_fluid_value(bitset.value.to_ullong());
            auto mask = get_fluid_value(bitset.mask.to_ullong());
            return new Type(value, mask);
        }

        // Unmasked
        template<
            class Type,
            typename std::enable_if_t<not IsMasked<Type>::value>* = nullptr
        >
        static MaskedBitSet to_bitset(const Type* object) {
            auto value = get_integer_value(object->value());
            return MaskedBitSet(value);
        }

        template<
            class Type,
            typename std::enable_if_t<not IsMasked<Type>::value>* = nullptr
        >
        static Type* from_bitset(MaskedBitSet&& bitset) {
            auto value = get_fluid_value(bitset.value.to_ullong());
            return new Type(value);
        }

        // Internal value converters
        static ValueType get_fluid_value(uint64_t);
        static uint64_t get_integer_value(const ValueType&);

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

    using End = Base<fluid_msg::of13::UDPDst, 0u, UDPDst>;
    static constexpr uint32_t HEADER_SIZE = End::OFFSET;

};

class BitVectorBridge
{
public:
    explicit BitVectorBridge(const BitVector& bit_vector):
        bit_vector_(bit_vector) {}
    explicit BitVectorBridge(BitVector&& bit_vector):
        bit_vector_(std::move(bit_vector)) {}

    BitVector copyBitVector() const {
        return bit_vector_;
    }
    BitVector popBitVector() {
        return std::move(bit_vector_);
    }

    template<class Map>
    typename Map::FieldType* getField() const {
        auto bitset = get_bits<Map>();
        return std::move(Map::fromBitSet(std::move(bitset)));
    }

    template<class Map>
    void setField(const typename Map::FieldType* fluid_object) {
        auto bitset = Map::toBitSet(fluid_object);
        set_bits<Map>(std::move(bitset));
    }

private:
    BitVector bit_vector_;

    template<class Map>
    typename Map::MaskedBitSet get_bits() const {
        auto value = typename Map::BitSet();
        auto mask = typename Map::BitSet();
        for (size_t index = 0; index < Map::SIZE; index++) {
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
                throw std::invalid_argument("Parsing error: Bad bit vector");
            }
        }
        return {std::move(value), std::move(mask)};
    }

    template<class Map>
    void set_bits(typename Map::MaskedBitSet bitset) {
        for (size_t index = 0; index < Map::SIZE; index++) {
            if (true == bitset.mask[index]) {
                auto bit_value = (true == bitset.value[index])
                                 ? BitValue::ONE
                                 : BitValue::ZERO;
                bit_vector_.setBit(Map::OFFSET + index, bit_value);
            }
        }
    }

};
