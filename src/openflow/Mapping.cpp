#include "Mapping.hpp"

using namespace fluid_msg;

static uint64_t get_eth_address_value(const EthAddress& eth_address)
{
    auto data = eth_address.get_data();
    auto raw_value = *reinterpret_cast<const uint64_t*>(data);
    auto value = ntoh64(raw_value) & 0x0000FFFFFFFFFFFF;
    return value;
}

static uint64_t get_ipv4_address_value(const IPAddress& ipv4_address)
{
    return ntoh32(ipv4_address.getIPv4());
}

template<>
uint64_t Mapping::EthSrc::get_value(const ValueType& eth_address)
{
    return get_eth_address_value(eth_address);
}

template<>
uint64_t Mapping::EthDst::get_value(const ValueType& eth_address)
{
    return get_eth_address_value(eth_address);
}

template<>
uint64_t Mapping::EthType::get_value(const ValueType& eth_type)
{
    return (0x0800 == eth_type) ? 1u : 0u;
}

template<>
uint64_t Mapping::IPProto::get_value(const ValueType& ip_proto)
{
    // 0x06 == TCP, 0x11 == UDP
    return (0x06 == ip_proto) ? 1u : 0u;
}

template<>
uint64_t Mapping::IPv4Src::get_value(const ValueType& ipv4_address)
{
    return get_ipv4_address_value(ipv4_address);
}

template<>
uint64_t Mapping::IPv4Dst::get_value(const ValueType& ipv4_address)
{
    return get_ipv4_address_value(ipv4_address);
}

template<>
uint64_t Mapping::TCPSrc::get_value(const ValueType& src_port)
{
    return hton16(src_port);
}

template<>
uint64_t Mapping::TCPDst::get_value(const ValueType& dst_port)
{
    return hton16(dst_port);
}
template<>
uint64_t Mapping::UDPSrc::get_value(const ValueType& src_port)
{
    return hton16(src_port);
}

template<>
uint64_t Mapping::UDPDst::get_value(const ValueType& dst_port)
{
    return hton16(dst_port);
}

//static EthAddress get

/*Mapping::EthSrc::MaskedBitSet
toBitSet(Mapping::EthSrc::FieldType* fluid_object)
{
    auto value = get_value(fluid_object->value());
    if (fluid_object->has_mask()) {
        auto mask = get_value(fluid_object->mask());
        return {value, mask};
    }
    else {
        return {value};
    }
}

Mapping::EthSrc::FieldType*
fromBitSet(Mapping::EthSrc::MaskedBitSet bitset)
{

}

BitVectorBridge::EthAddrBitSet
BitVectorBridge::to_bitset(const EthAddress& eth_address) const
{
    auto data = eth_address.get_data();
    auto raw_value = *reinterpret_cast<const uint64_t*>(data);
    auto value = ntoh64(raw_value) & 0x0000FFFFFFFFFFFF;
    return EthAddrBitSet(value);
}

EthAddress
BitVectorBridge::get_eth_address(const EthAddrBitSet& bitset) const
{
    uint64_t data = hton64(bitset.to_ullong());
    auto raw_value = reinterpret_cast<const uint8_t*>(&data);
    return EthAddress(raw_value);
}

BitVectorBridge::IPv4AddrBitSet
BitVectorBridge::to_bitset(const IPAddress& ipv4_address) const
{
    auto value = ntoh32(ipv4_address.getIPv4());
    return IPv4AddrBitSet(value);
}

IPAddress
BitVectorBridge::get_ip_address(const IPv4AddrBitSet& bitset) const
{

}*/

/*
of13::OXMTLV BitVectorBridge::getField(uint8_t type) const
{
    switch (type) {
    case of13::OFPXMT_OFB_ETH_DST:
        break;
    case of13::OFPXMT_OFB_ETH_SRC:
        break;
    case of13::OFPXMT_OFB_ETH_TYPE:
        break;
    case of13::OFPXMT_OFB_IP_PROTO:
        break;
    case of13::OFPXMT_OFB_IPV4_SRC:
        break;
    case of13::OFPXMT_OFB_IPV4_DST:
        break;
    case of13::OFPXMT_OFB_TCP_SRC:
        break;
    case of13::OFPXMT_OFB_TCP_DST:
        break;
    case of13::OFPXMT_OFB_UDP_SRC:
        break;
    case of13::OFPXMT_OFB_UDP_DST:
        break;
    default:
        std::cerr << "Unsupported OXMTLV type" << std::endl;
        break;
    }
}

of13::EthSrc* BitVectorBridge::getEthSrc() const
{
    auto value = EthAddrBitSet();
    auto mask = EthAddrBitSet();
    for (uint32_t index = 0; index < Size::ETH_SRC; index++) {
        auto bit_value = bit_vector_.getBit(Offset::ETH_SRC + index);
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
    return new of13::EthSrc(get_eth_address(value), get_eth_address(mask));
}

void BitVectorBridge::setEthSrc(of13::EthSrc* eth_src)
{
    auto value = to_bitset(eth_src->value());
    auto mask = to_bitset(eth_src->mask());
    for (uint32_t index = 0; index < Size::ETH_SRC; index++) {
        if (true == mask[index]) {
            auto bit_value = (true == value[index]) ? BitValue::ONE
                                                    : BitValue::ZERO;
            bit_vector_.setBit(Offset::ETH_SRC + index, bit_value);
        }
    }
}

of13::EthDst* BitVectorBridge::getEthDst() const
{

}

void BitVectorBridge::setEthDst(of13::EthDst* eth_dst)
{
    auto value = to_bitset(eth_dst->value());
    auto mask = to_bitset(eth_dst->mask());
    for (uint32_t index = 0; index < Size::ETH_DST; index++) {
        if (true == mask[index]) {
            auto bit_value = (true == value[index]) ? BitValue::ONE
                                                    : BitValue::ZERO;
            bit_vector_.setBit(Offset::ETH_DST + index, bit_value);
        }
    }
}

void BitVectorBridge::setField(of13::EthType* eth_type)
{

}

void BitVectorBridge::setField(of13::IPProto* ip_proto)
{

}

void BitVectorBridge::setField(of13::IPv4Src* ipv4_src)
{

}

void BitVectorBridge::setField(of13::IPv4Dst* ipv4_dst)
{

}

void BitVectorBridge::setField(of13::TCPSrc* tcp_src)
{

}

void BitVectorBridge::setField(of13::TCPDst* tcp_dst)
{

}

void BitVectorBridge::setField(of13::UDPSrc* udp_src)
{

}

void BitVectorBridge::setField(of13::UDPDst* udp_dst)
{

}

template<class BitSetType>
MaskedValue<BitSetType> BitVectorBridge::get_masked_value(uint32_t offset,
                                                          uint32_t size)
{
    auto value = BitSetType();
    auto mask = BitSetType();
    for (uint32_t index = 0; index < size; index++) {
        auto bit_value = bit_vector_.getBit(offset + index);
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

template<class BitSetType>
void BitVectorBridge::set_masked_value(uint32_t offset, uint32_t size,
                                       MaskedValue<BitSetType>&& masked_value)
{
    auto& value = masked_value.value;
    auto& mask = masked_value.mask;
    for (uint32_t index = 0; index < size; index++) {
        if (true == mask[index]) {
            auto bit_value = (true == value[index]) ? BitValue::ONE
                                                    : BitValue::ZERO;
            bit_vector_.setBit(offset + index, bit_value);
        }
    }
}

BitVectorBridge::EthAddrBitSet
BitVectorBridge::to_bitset(const EthAddress& eth_address) const
{
    auto data = eth_address.get_data();
    auto raw_value = *reinterpret_cast<const uint64_t*>(data);
    auto value = ntoh64(raw_value) & 0x0000FFFFFFFFFFFF;
    return EthAddrBitSet(value);
}

EthAddress
BitVectorBridge::get_eth_address(const EthAddrBitSet& bitset) const
{
    uint64_t data = hton64(bitset.to_ullong());
    auto raw_value = reinterpret_cast<const uint8_t*>(&data);
    return EthAddress(raw_value);
}

BitVectorBridge::IPv4AddrBitSet
BitVectorBridge::to_bitset(const IPAddress& ipv4_address) const
{
    auto value = ntoh32(ipv4_address.getIPv4());
    return IPv4AddrBitSet(value);
}

IPAddress
BitVectorBridge::get_ip_address(const IPv4AddrBitSet& bitset) const
{

}
*/