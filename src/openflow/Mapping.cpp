#include "Mapping.hpp"

// Static helper functions
static uint64_t get_eth_value(const fluid_msg::EthAddress& eth_address)
{
    auto data = eth_address.get_data();
    auto raw_value = *reinterpret_cast<const uint64_t*>(data);
    auto value = (ntoh64(raw_value) & 0xFFFFFFFFFFFF0000) >> 16;
    return value;
}

static fluid_msg::EthAddress get_eth_fluid_value(uint64_t value)
{
    auto raw_value = hton64(value) >> 16;
    auto data = reinterpret_cast<uint8_t*>(&raw_value);
    return std::move(fluid_msg::EthAddress(data));
}

static uint64_t get_ipv4_value(const fluid_msg::IPAddress& ipv4_address)
{
    return ntoh32(ipv4_address.getIPv4());
}

static fluid_msg::IPAddress get_ipv4_fluid_value(uint32_t value)
{
    auto raw_value = hton32(value);
    return std::move(fluid_msg::IPAddress(raw_value));
}

// L2
template<>
Mapping::EthSrc::ValueType
Mapping::EthSrc::get_fluid_value(uint64_t eth_address)
{
    return std::move(get_eth_fluid_value(eth_address));
}

template<>
uint64_t Mapping::EthSrc::get_integer_value(const ValueType& eth_address)
{
    return get_eth_value(eth_address);
}

template<>
Mapping::EthDst::ValueType
Mapping::EthDst::get_fluid_value(uint64_t eth_address)
{
    return std::move(get_eth_fluid_value(eth_address));
}

template<>
uint64_t Mapping::EthDst::get_integer_value(const ValueType& eth_address)
{
    return get_eth_value(eth_address);
}

template<>
Mapping::EthType::ValueType
Mapping::EthType::get_fluid_value(uint64_t eth_type)
{
    // TODO: use all bits if ether type
    return (1u == eth_type) ? ETH_TYPE::IPv4 : 0x0;
}

template<>
uint64_t Mapping::EthType::get_integer_value(const ValueType& eth_type)
{
    // TODO: check if eth_type is in network order - then ntoh8() it
    return (ETH_TYPE::IPv4 == eth_type) ? 1u : 0u;
}

// L3
template<>
Mapping::IPProto::ValueType
Mapping::IPProto::get_fluid_value(uint64_t ip_proto)
{
    // TODO: use all bits if ip proto
    return (1u == ip_proto) ? IP_PROTO::TCP : IP_PROTO::UDP;
}

template<>
uint64_t Mapping::IPProto::get_integer_value(const ValueType& ip_proto)
{
    return (IP_PROTO::TCP == ip_proto) ? 1u : 0u;
}

template<>
Mapping::IPv4Src::ValueType
Mapping::IPv4Src::get_fluid_value(uint64_t ipv4_address)
{
    auto value = static_cast<uint32_t>(ipv4_address);
    return std::move(get_ipv4_fluid_value(value));
}

template<>
uint64_t Mapping::IPv4Src::get_integer_value(const ValueType& ipv4_address)
{
    return get_ipv4_value(ipv4_address);
}

template<>
Mapping::IPv4Dst::ValueType
Mapping::IPv4Dst::get_fluid_value(uint64_t ipv4_address)
{
    auto value = static_cast<uint32_t>(ipv4_address);
    return std::move(get_ipv4_fluid_value(value));
}

template<>
uint64_t Mapping::IPv4Dst::get_integer_value(const ValueType& ipv4_address)
{
    return get_ipv4_value(ipv4_address);
}

// L4
template<>
Mapping::TCPSrc::ValueType
Mapping::TCPSrc::get_fluid_value(uint64_t port)
{
    return ntoh16(static_cast<uint16_t>(port));
}

template<>
uint64_t Mapping::TCPSrc::get_integer_value(const ValueType& port)
{
    return hton16(port);
}

template<>
Mapping::TCPDst::ValueType
Mapping::TCPDst::get_fluid_value(uint64_t port)
{
    return ntoh16(static_cast<uint16_t>(port));
}

template<>
uint64_t Mapping::TCPDst::get_integer_value(const ValueType& port)
{
    return hton16(port);
}

template<>
Mapping::UDPSrc::ValueType
Mapping::UDPSrc::get_fluid_value(uint64_t port)
{
    return ntoh16(static_cast<uint16_t>(port));
}

template<>
uint64_t Mapping::UDPSrc::get_integer_value(const ValueType& port)
{
    return hton16(port);
}

template<>
Mapping::UDPDst::ValueType
Mapping::UDPDst::get_fluid_value(uint64_t port)
{
    return ntoh16(static_cast<uint16_t>(port));
}

template<>
uint64_t Mapping::UDPDst::get_integer_value(const ValueType& port)
{
    return hton16(port);
}
