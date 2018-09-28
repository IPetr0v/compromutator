#include "gtest/gtest.h"

#include "../src/Proto.hpp"
#include "../src/openflow/Parser.hpp"

#include <fluid/ofcommon/msg.hh>
#include <fluid/of13/openflow-13.h>
#include <fluid/of13msg.hh>

using namespace fluid_msg;
using namespace proto;

struct ParserTest : public ::testing::Test,
                    public Parser
{
    of13::Match crossParseMatch(of13::Match match) const {
        return Parser::get_of_match(Parser::get_match(match));
    }

    ActionsBase parseActions(of13::InstructionSet instructions) const {
        return Parser::get_actions(instructions);
    }

    void setEthernet(of13::Match& match) const {
        match.add_oxm_field(new of13::InPort(in_port));
        match.add_oxm_field(new of13::EthSrc(eth_src));
        match.add_oxm_field(new of13::EthDst(eth_dst));
    }

    void setMaskedEthernet(of13::Match& match) const {
        match.add_oxm_field(new of13::InPort(in_port));
        match.add_oxm_field(new of13::EthSrc(eth_src, eth_src_mask));
        match.add_oxm_field(new of13::EthDst(eth_dst, eth_dst_mask));
    }

    void setIPv4(of13::Match& match) const {
        match.add_oxm_field(new of13::EthType(Ethernet::TYPE::IPv4));
        match.add_oxm_field(new of13::IPv4Src(ip_src));
        match.add_oxm_field(new of13::IPv4Dst(ip_dst));
    }

    void setMaskedIPv4(of13::Match& match) const {
        match.add_oxm_field(new of13::EthType(Ethernet::TYPE::IPv4));
        match.add_oxm_field(new of13::IPv4Src(ip_src, ip_src_mask));
        match.add_oxm_field(new of13::IPv4Dst(ip_dst, ip_dst_mask));
    }

    const uint32_t in_port = 9u;

    const EthAddress eth_src          = EthAddress("01:23:45:67:89:AB");
    const EthAddress eth_src_mask     = EthAddress("00:FF:01:00:FF:FF");
    const EthAddress eth_src_wildcard = EthAddress("00:23:01:00:89:AB");
    const EthAddress eth_dst          = EthAddress("AB:CD:FE:AB:CD:FE");
    const EthAddress eth_dst_mask     = EthAddress("FF:00:FF:FF:00:01");
    const EthAddress eth_dst_wildcard = EthAddress("AB:00:FE:AB:00:00");
    const EthAddress eth_full_mask    = EthAddress("FF:FF:FF:FF:FF:FF");

    const IPAddress ip_src          = IPAddress("1.2.3.4");
    const IPAddress ip_src_mask     = IPAddress("0.0.255.255");
    const IPAddress ip_src_wildcard = IPAddress("0.0.3.4");
    const IPAddress ip_dst          = IPAddress("5.6.7.8");
    const IPAddress ip_dst_mask     = IPAddress("255.255.0.0");
    const IPAddress ip_dst_wildcard = IPAddress("5.6.0.0");
    const IPAddress ip_full_mask    = IPAddress("255.255.255.255");

    const uint16_t transport_src = 23u;
    const uint16_t transport_dst = 8080u;

    const uint32_t out_group = 1u;
    const uint32_t out_port = 2u;
    const uint8_t out_table = 3u;
};

TEST_F(ParserTest, BasicMatchTest)
{
    // Create match
    of13::Match match;
    auto cross_match = crossParseMatch(match);

    // Check fields
    EXPECT_EQ(nullptr, cross_match.in_port());
    EXPECT_EQ(nullptr, cross_match.eth_src());
    EXPECT_EQ(nullptr, cross_match.eth_dst());
    EXPECT_EQ(nullptr, cross_match.eth_type());
    EXPECT_EQ(nullptr, cross_match.vlan_vid());
    EXPECT_EQ(nullptr, cross_match.vlan_pcp());
    EXPECT_EQ(nullptr, cross_match.ip_proto());
    EXPECT_EQ(nullptr, cross_match.ipv4_src());
    EXPECT_EQ(nullptr, cross_match.ipv4_dst());
    EXPECT_EQ(nullptr, cross_match.ipv6_src());
    EXPECT_EQ(nullptr, cross_match.ipv6_dst());
    EXPECT_EQ(nullptr, cross_match.arp_op());
    EXPECT_EQ(nullptr, cross_match.arp_sha());
    EXPECT_EQ(nullptr, cross_match.arp_spa());
    EXPECT_EQ(nullptr, cross_match.arp_tha());
    EXPECT_EQ(nullptr, cross_match.arp_tpa());
    EXPECT_EQ(nullptr, cross_match.icmpv4_type());
    EXPECT_EQ(nullptr, cross_match.icmpv4_code());
    EXPECT_EQ(nullptr, cross_match.icmpv6_type());
    EXPECT_EQ(nullptr, cross_match.icmpv6_code());
    EXPECT_EQ(nullptr, cross_match.tcp_src());
    EXPECT_EQ(nullptr, cross_match.tcp_dst());
    EXPECT_EQ(nullptr, cross_match.udp_src());
    EXPECT_EQ(nullptr, cross_match.udp_dst());
}

TEST_F(ParserTest, EthernetTest)
{
    // Create match
    of13::Match match;
    setEthernet(match);

    // Parse match
    auto cross_match = crossParseMatch(match);
    ASSERT_NE(nullptr, cross_match.in_port());
    ASSERT_NE(nullptr, cross_match.eth_src());
    ASSERT_NE(nullptr, cross_match.eth_dst());

    // Check fields
    EXPECT_EQ(in_port, cross_match.in_port()->value());
    EXPECT_EQ(eth_src, cross_match.eth_src()->value());
    EXPECT_EQ(eth_full_mask, cross_match.eth_src()->mask());
    EXPECT_EQ(eth_dst, cross_match.eth_dst()->value());
    EXPECT_EQ(eth_full_mask, cross_match.eth_dst()->mask());
    EXPECT_EQ(nullptr, cross_match.eth_type());
    EXPECT_EQ(nullptr, cross_match.ip_proto());
    EXPECT_EQ(nullptr, cross_match.ipv4_src());
    EXPECT_EQ(nullptr, cross_match.ipv4_src());
    EXPECT_EQ(nullptr, cross_match.ipv4_dst());
    EXPECT_EQ(nullptr, cross_match.ipv4_dst());
    EXPECT_EQ(nullptr, cross_match.tcp_src());
    EXPECT_EQ(nullptr, cross_match.tcp_dst());
    EXPECT_EQ(nullptr, cross_match.udp_src());
    EXPECT_EQ(nullptr, cross_match.udp_dst());
}

TEST_F(ParserTest, EthernetMaskedTest)
{
    // Create match
    of13::Match match;
    setMaskedEthernet(match);

    // Check fields
    auto cross_match = crossParseMatch(match);
    EXPECT_EQ(in_port, cross_match.in_port()->value());
    EXPECT_EQ(eth_src_wildcard, cross_match.eth_src()->value());
    EXPECT_EQ(eth_src_mask, cross_match.eth_src()->mask());
    EXPECT_EQ(eth_dst_wildcard, cross_match.eth_dst()->value());
    EXPECT_EQ(eth_dst_mask, cross_match.eth_dst()->mask());
}

TEST_F(ParserTest, IPv4Test)
{
    // Create match
    of13::Match match;
    setIPv4(match);

    // Parse match
    auto cross_match = crossParseMatch(match);
    ASSERT_NE(nullptr, cross_match.eth_type());
    ASSERT_NE(nullptr, cross_match.ipv4_src());
    ASSERT_NE(nullptr, cross_match.ipv4_dst());

    // Check fields
    EXPECT_EQ(nullptr, cross_match.in_port());
    EXPECT_EQ(nullptr, cross_match.eth_src());
    EXPECT_EQ(nullptr, cross_match.eth_dst());
    EXPECT_EQ(Ethernet::TYPE::IPv4, cross_match.eth_type()->value());
    EXPECT_EQ(ip_src, cross_match.ipv4_src()->value());
    EXPECT_EQ(ip_full_mask, cross_match.ipv4_src()->mask());
    EXPECT_EQ(ip_dst, cross_match.ipv4_dst()->value());
    EXPECT_EQ(ip_full_mask, cross_match.ipv4_dst()->mask());
    EXPECT_EQ(nullptr, cross_match.ip_proto());
    EXPECT_EQ(nullptr, cross_match.tcp_src());
    EXPECT_EQ(nullptr, cross_match.tcp_dst());
    EXPECT_EQ(nullptr, cross_match.udp_src());
    EXPECT_EQ(nullptr, cross_match.udp_dst());
}

TEST_F(ParserTest, IPv4MaskedTest)
{
    // Create match
    of13::Match match;
    setMaskedEthernet(match);
    setMaskedIPv4(match);

    // Check fields
    auto cross_match = crossParseMatch(match);
    EXPECT_EQ(Ethernet::TYPE::IPv4, cross_match.eth_type()->value());
    EXPECT_EQ(ip_src_wildcard, cross_match.ipv4_src()->value());
    EXPECT_EQ(ip_src_mask, cross_match.ipv4_src()->mask());
    EXPECT_EQ(ip_dst_wildcard, cross_match.ipv4_dst()->value());
    EXPECT_EQ(ip_dst_mask, cross_match.ipv4_dst()->mask());
}

TEST_F(ParserTest, ARPTest)
{
    // Create match
    of13::Match match;
    match.add_oxm_field(new of13::EthType(Ethernet::TYPE::ARP));

    // Check fields
    auto cross_match = crossParseMatch(match);
    EXPECT_EQ(Ethernet::TYPE::ARP, cross_match.eth_type()->value());
}

TEST_F(ParserTest, LLDPTest)
{
    // Create match
    of13::Match match;
    match.add_oxm_field(new of13::EthType(Ethernet::TYPE::LLDP));

    // Check fields
    auto cross_match = crossParseMatch(match);
    EXPECT_EQ(Ethernet::TYPE::LLDP, cross_match.eth_type()->value());
}

TEST_F(ParserTest, TCPTest)
{
    // Create match
    of13::Match match;
    match.add_oxm_field(new of13::EthType(Ethernet::TYPE::IPv4));
    match.add_oxm_field(new of13::IPProto(IPv4::PROTO::TCP));
    match.add_oxm_field(new of13::TCPSrc(transport_src));
    match.add_oxm_field(new of13::TCPDst(transport_dst));

    // Parse match
    auto cross_match = crossParseMatch(match);
    ASSERT_NE(nullptr, cross_match.eth_type());
    ASSERT_NE(nullptr, cross_match.ip_proto());
    ASSERT_NE(nullptr, cross_match.tcp_src());
    ASSERT_NE(nullptr, cross_match.tcp_dst());

    // Check fields
    EXPECT_EQ(nullptr, cross_match.in_port());
    EXPECT_EQ(nullptr, cross_match.eth_src());
    EXPECT_EQ(nullptr, cross_match.eth_dst());
    EXPECT_EQ(Ethernet::TYPE::IPv4, cross_match.eth_type()->value());
    EXPECT_EQ(nullptr, cross_match.ipv4_src());
    EXPECT_EQ(nullptr, cross_match.ipv4_dst());
    EXPECT_EQ(IPv4::PROTO::TCP, cross_match.ip_proto()->value());
    EXPECT_EQ(transport_src, cross_match.tcp_src()->value());
    EXPECT_EQ(transport_dst, cross_match.tcp_dst()->value());
    EXPECT_EQ(nullptr, cross_match.udp_src());
    EXPECT_EQ(nullptr, cross_match.udp_dst());
}

TEST_F(ParserTest, UDPTest)
{
    // Create match
    of13::Match match;
    match.add_oxm_field(new of13::EthType(Ethernet::TYPE::IPv4));
    match.add_oxm_field(new of13::IPProto(IPv4::PROTO::UDP));
    match.add_oxm_field(new of13::TCPSrc(transport_src));
    match.add_oxm_field(new of13::TCPDst(transport_dst));

    // Check fields
    auto cross_match = crossParseMatch(match);
    EXPECT_EQ(Ethernet::TYPE::IPv4, cross_match.eth_type()->value());
    EXPECT_EQ(IPv4::PROTO::UDP, cross_match.ip_proto()->value());
    EXPECT_EQ(nullptr, cross_match.tcp_src());
    EXPECT_EQ(nullptr, cross_match.tcp_dst());
    EXPECT_EQ(transport_src, cross_match.udp_src()->value());
    EXPECT_EQ(transport_dst, cross_match.udp_dst()->value());
}

TEST_F(ParserTest, DropActionTest)
{
    // Create actions
    of13::InstructionSet instructions;

    // Check parsed actions
    auto actions_base = parseActions(instructions);
    ASSERT_EQ(1u, actions_base.port_actions.size());
    auto& port_action = actions_base.port_actions.back();
    EXPECT_EQ(PortType::DROP, port_action.port_type);
    EXPECT_EQ(SpecialPort::NONE, port_action.port_id);
}

TEST_F(ParserTest, ControllerActionTest)
{
    // Create actions
    of13::InstructionSet instructions;
    of13::ApplyActions actions;
    actions.add_action(new of13::OutputAction(of13::OFPP_CONTROLLER, 2048));
    instructions.add_instruction(actions);

    // Check parsed actions
    auto actions_base = parseActions(instructions);
    ASSERT_EQ(1u, actions_base.port_actions.size());
    auto& port_action = actions_base.port_actions.back();
    EXPECT_EQ(PortType::CONTROLLER, port_action.port_type);
    EXPECT_EQ(SpecialPort::NONE, port_action.port_id);
}

TEST_F(ParserTest, BasicActionTest)
{
    // Create actions
    of13::InstructionSet instructions;
    of13::ApplyActions actions;
    actions.add_action(new of13::GroupAction(out_group));
    actions.add_action(new of13::OutputAction(out_port, 2048));
    of13::GoToTable table_instruction(out_table);
    instructions.add_instruction(actions);
    instructions.add_instruction(table_instruction);

    // Check parsed actions
    auto actions_base = parseActions(instructions);

    ASSERT_EQ(1u, actions_base.group_actions.size());
    auto& group_action = actions_base.group_actions.back();
    EXPECT_EQ(out_group, group_action.group_id);

    ASSERT_EQ(1u, actions_base.port_actions.size());
    auto& port_action = actions_base.port_actions.back();
    EXPECT_EQ(PortType::NORMAL, port_action.port_type);
    EXPECT_EQ(out_port, port_action.port_id);

    ASSERT_EQ(1u, actions_base.table_actions.size());
    auto& table_action = actions_base.table_actions.back();
    EXPECT_EQ(out_table, table_action.table_id);
}
