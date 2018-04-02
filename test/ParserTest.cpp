#include "gtest/gtest.h"

#include "../src/openflow/Parser.hpp"

#include <fluid/ofcommon/msg.hh>
#include <fluid/of13/openflow-13.h>
#include <fluid/of13msg.hh>

using namespace fluid_msg;

struct ParserTest : public ::testing::Test,
                    public Parser
{
    of13::Match crossParseMatch(of13::Match match) const {
        return Parser::get_of_match(Parser::get_match(match));
    }

    const uint32_t in_port = 9u;
    const EthAddress eth_src = EthAddress("01:23:45:67:89:AB");
    const EthAddress eth_src_mask = EthAddress("00:FF:01:00:FF:FF");
    const EthAddress eth_src_wildcard = EthAddress("00:23:01:00:89:AB");
    const EthAddress eth_dst = EthAddress("AB:CD:FE:AB:CD:FE");
    const EthAddress eth_dst_mask = EthAddress("FF:00:FF:FF:00:01");
    const EthAddress eth_dst_wildcard = EthAddress("AB:00:FE:AB:00:00");
    const EthAddress eth_full_mask = EthAddress("FF:FF:FF:FF:FF:FF");
    const uint16_t eth_type_ip = 0x0800;

    const uint8_t ip_proto_tcp = 0x06;
    const uint8_t ip_proto_udp = 0x11;
    const IPAddress ip_src = IPAddress("1.2.3.4");
    const IPAddress ip_src_mask = IPAddress("0.0.255.255");
    const IPAddress ip_src_wildcard = IPAddress("0.0.3.4");
    const IPAddress ip_dst = IPAddress("5.6.7.8");
    const IPAddress ip_dst_mask = IPAddress("255.255.0.0");
    const IPAddress ip_dst_wildcard = IPAddress("5.6.0.0");
    const IPAddress ip_full_mask = IPAddress("255.255.255.255");

    const uint16_t transport_src = 23u;
    const uint16_t transport_dst = 8080u;
};

TEST_F(ParserTest, MatchTcpUnmaskedTest)
{
    // Create match
    of13::Match match;
    match.add_oxm_field(new of13::InPort(in_port));
    match.add_oxm_field(new of13::EthSrc(eth_src, eth_full_mask));
    match.add_oxm_field(new of13::EthDst(eth_dst, eth_full_mask));
    match.add_oxm_field(new of13::EthType(eth_type_ip));
    match.add_oxm_field(new of13::IPProto(ip_proto_tcp));
    match.add_oxm_field(new of13::IPv4Src(ip_src, ip_full_mask));
    match.add_oxm_field(new of13::IPv4Dst(ip_dst, ip_full_mask));
    match.add_oxm_field(new of13::TCPSrc(transport_src));
    match.add_oxm_field(new of13::TCPDst(transport_dst));

    // Parse match
    auto cross_match = crossParseMatch(match);
    ASSERT_NE(nullptr, cross_match.in_port());
    ASSERT_NE(nullptr, cross_match.eth_src());
    ASSERT_NE(nullptr, cross_match.eth_dst());
    ASSERT_NE(nullptr, cross_match.eth_type());
    ASSERT_NE(nullptr, cross_match.ip_proto());
    ASSERT_NE(nullptr, cross_match.ipv4_src());
    ASSERT_NE(nullptr, cross_match.ipv4_dst());
    ASSERT_NE(nullptr, cross_match.tcp_src());
    ASSERT_NE(nullptr, cross_match.tcp_dst());

    // Check fields
    EXPECT_EQ(in_port, cross_match.in_port()->value());
    EXPECT_EQ(eth_src, cross_match.eth_src()->value());
    EXPECT_EQ(eth_full_mask, cross_match.eth_src()->mask());
    EXPECT_EQ(eth_dst, cross_match.eth_dst()->value());
    EXPECT_EQ(eth_full_mask, cross_match.eth_dst()->mask());
    EXPECT_EQ(eth_type_ip, cross_match.eth_type()->value());
    EXPECT_EQ(ip_proto_tcp, cross_match.ip_proto()->value());
    EXPECT_EQ(ip_src, cross_match.ipv4_src()->value());
    EXPECT_EQ(ip_full_mask, cross_match.ipv4_src()->mask());
    EXPECT_EQ(ip_dst, cross_match.ipv4_dst()->value());
    EXPECT_EQ(ip_full_mask, cross_match.ipv4_dst()->mask());
    EXPECT_EQ(transport_src, cross_match.tcp_src()->value());
    EXPECT_EQ(transport_dst, cross_match.tcp_dst()->value());
    EXPECT_EQ(nullptr, cross_match.udp_src());
    EXPECT_EQ(nullptr, cross_match.udp_dst());
}

TEST_F(ParserTest, MatchTcpMaskedTest)
{
    // Create match
    of13::Match match;
    match.add_oxm_field(new of13::InPort(in_port));
    match.add_oxm_field(new of13::EthSrc(eth_src, eth_src_mask));
    match.add_oxm_field(new of13::EthDst(eth_dst, eth_dst_mask));
    match.add_oxm_field(new of13::EthType(eth_type_ip));
    match.add_oxm_field(new of13::IPProto(ip_proto_tcp));
    match.add_oxm_field(new of13::IPv4Src(ip_src, ip_src_mask));
    match.add_oxm_field(new of13::IPv4Dst(ip_dst, ip_dst_mask));
    match.add_oxm_field(new of13::TCPSrc(transport_src));
    match.add_oxm_field(new of13::TCPDst(transport_dst));

    // Parse match
    auto cross_match = crossParseMatch(match);
    ASSERT_NE(nullptr, cross_match.in_port());
    ASSERT_NE(nullptr, cross_match.eth_src());
    ASSERT_NE(nullptr, cross_match.eth_dst());
    ASSERT_NE(nullptr, cross_match.eth_type());
    ASSERT_NE(nullptr, cross_match.ip_proto());
    ASSERT_NE(nullptr, cross_match.ipv4_src());
    ASSERT_NE(nullptr, cross_match.ipv4_dst());
    ASSERT_NE(nullptr, cross_match.tcp_src());
    ASSERT_NE(nullptr, cross_match.tcp_dst());

    // Check fields
    EXPECT_EQ(in_port, cross_match.in_port()->value());
    EXPECT_EQ(eth_src_wildcard, cross_match.eth_src()->value());
    EXPECT_EQ(eth_src_mask, cross_match.eth_src()->mask());
    EXPECT_EQ(eth_dst_wildcard, cross_match.eth_dst()->value());
    EXPECT_EQ(eth_dst_mask, cross_match.eth_dst()->mask());
    EXPECT_EQ(eth_type_ip, cross_match.eth_type()->value());
    EXPECT_EQ(ip_proto_tcp, cross_match.ip_proto()->value());
    EXPECT_EQ(ip_src_wildcard, cross_match.ipv4_src()->value());
    EXPECT_EQ(ip_src_mask, cross_match.ipv4_src()->mask());
    EXPECT_EQ(ip_dst_wildcard, cross_match.ipv4_dst()->value());
    EXPECT_EQ(ip_dst_mask, cross_match.ipv4_dst()->mask());
    EXPECT_EQ(transport_src, cross_match.tcp_src()->value());
    EXPECT_EQ(transport_dst, cross_match.tcp_dst()->value());
    EXPECT_EQ(nullptr, cross_match.udp_src());
    EXPECT_EQ(nullptr, cross_match.udp_dst());
}
