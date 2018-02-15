#include "Parser.hpp"

NetworkSpace Parser::get_domain(of13::Match match) const
{
    PortId in_port = SpecialPort::ANY;
    auto bit_vector = BitVectorBridge(BitVector::wholeSpace(150u));

    // Input port
    if (match.in_port()) {
        in_port = match.in_port()->value();
    }

    // Ethernet
    if (match.eth_src()) {
        bit_vector.setField<Mapping::EthSrc>(match.eth_src());
    }
    if (match.eth_dst()) {
        bit_vector.setField<Mapping::EthDst>(match.eth_dst());
    }
    if (match.eth_type()) {
        bit_vector.setField<Mapping::EthType>(match.eth_type());
    }

    // IPv4
    if (match.ip_proto()) {
        bit_vector.setField<Mapping::IPProto>(match.ip_proto());
    }
    if (match.ipv4_src()) {
        bit_vector.setField<Mapping::IPv4Src>(match.ipv4_src());
    }
    if (match.ipv4_dst()) {
        bit_vector.setField<Mapping::IPv4Dst>(match.ipv4_dst());
    }

    // TCP
    if (match.tcp_src()) {
        bit_vector.setField<Mapping::TCPSrc>(match.tcp_src());
    }
    if (match.tcp_dst()) {
        bit_vector.setField<Mapping::TCPDst>(match.tcp_dst());
    }

    // UDP
    if (match.udp_src()) {
        bit_vector.setField<Mapping::UDPSrc>(match.udp_src());
    }
    if (match.udp_dst()) {
        bit_vector.setField<Mapping::UDPDst>(match.udp_dst());
    }

    return {in_port, HeaderSpace(bit_vector.popBitVector())};
/*
    // Ethernet
    if (match.eth_src()) {
        auto eth_src = get_eth_address(
            match.eth_src()->value().get_data()
        );
        auto eth_src_mask = get_eth_address(
            match.eth_src()->mask().get_data()
        );
        for (uint32_t index = 0; index < Size::ETH_SRC; index++) {
            if (true == eth_src_mask[index]) {
                auto value = (true == eth_src[index]) ? BitValue::ONE
                                                      : BitValue::ZERO;
                bit_vector.setBit(Offset::ETH_SRC + index, value);
            }
        }
    }
    if (match.eth_dst()) {
        auto eth_dst = get_eth_address(
            match.eth_src()->value().get_data()
        );
        auto eth_dst_mask = get_eth_address(
            match.eth_src()->mask().get_data()
        );
        for (uint32_t index = 0; index < Size::ETH_DST; index++) {
            if (true == eth_dst_mask[index]) {
                auto value = (true == eth_dst[index]) ? BitValue::ONE
                                                      : BitValue::ZERO;
                bit_vector.setBit(Offset::ETH_DST + index, value);
            }
        }
    }
    if (match.eth_type()) {
        // TODO: change this to an arbitrary ethernet type
        if (0x0800 == match.eth_type()->value()) {
            bit_vector.setBit(Offset::ETH_TYPE, BitValue::ONE);
        }
        else {
            bit_vector.setBit(Offset::ETH_TYPE, BitValue::ZERO);
        }
    }

    // IPv4
    if (match.ipv4_src()) {
        auto ipv4_src = get_ipv4_address(
            ntoh32(match.ipv4_src()->value().getIPv4())
        );
        auto ipv4_src_mask = get_ipv4_address(
            ntoh32(match.ipv4_src()->mask().getIPv4())
        );
        for (uint32_t index = 0; index < Size::IPV4_SRC; index++) {
            if (true == ipv4_src_mask[index]) {
                auto value = (true == ipv4_src[index]) ? BitValue::ONE
                                                       : BitValue::ZERO;
                bit_vector.setBit(Offset::IPV4_SRC + index, value);
            }
        }
    }
    if (match.ipv4_dst()) {
        auto ipv4_dst = get_ipv4_address(
            match.ipv4_dst()->value().getIPv4()
        );
        auto ipv4_dst_mask = get_ipv4_address(
            match.ipv4_dst()->mask().getIPv4()
        );
        for (uint32_t index = 0; index < Size::IPV4_DST; index++) {
            if (true == ipv4_dst_mask[index]) {
                auto value = (true == ipv4_dst[index]) ? BitValue::ONE
                                                       : BitValue::ZERO;
                bit_vector.setBit(Offset::IPV4_DST + index, value);
            }
        }
    }
    if (match.ip_proto()) {

    }

    // TCP
    if (match.tcp_src()) {
        hton16(match.tcp_src()->value());
    }
    if (match.tcp_dst()) {
        hton16(match.tcp_dst()->value());
    }

    // UDP
    if (match.udp_src()) {
        hton16(match.udp_src()->value());
    }
    if (match.udp_dst()) {
        hton16(match.udp_dst()->value());
    }

    return {in_port, HeaderSpace(bit_vector)};*/
}

of13::Match Parser::get_match(const NetworkSpace& domain) const
{
    of13::Match match;

    return std::move(match);
}

ActionsBase Parser::get_actions(of13::InstructionSet instructions) const
{
    ActionsBase actions;
    for (auto instruction : instructions.instruction_set()) {
        switch (instruction->type()) {
        case of13::OFPIT_GOTO_TABLE: {
            auto goto_table = dynamic_cast<of13::GoToTable*>(instruction);
            actions.table_actions.emplace_back(goto_table->table_id());
            break;
        }
        case of13::OFPIT_WRITE_METADATA:
            break;
        case of13::OFPIT_WRITE_ACTIONS: {
            break;
        }
        case of13::OFPIT_APPLY_ACTIONS: {
            auto apply_actions = dynamic_cast<of13::ApplyActions*>(instruction);
            actions += get_apply_actions(apply_actions);
            break;
        }
        case of13::OFPIT_CLEAR_ACTIONS:
            break;
        case of13::OFPIT_METER:
            break;
        case of13::OFPIT_EXPERIMENTER:
            break;
        default:
            std::cerr << "Unknown instruction type" << std::endl;
            break;
        }
    }
    return std::move(actions);
}

ActionsBase Parser::get_apply_actions(of13::ApplyActions* apply_actions) const
{
    ActionsBase actions;
    for (auto action : apply_actions->actions().action_list()) {
        switch (action->type()) {
        case of13::OFPAT_OUTPUT: {
            //auto output_action = dynamic_cast<of13::OutputAction*>(action);
            break;
        }
        case of13::OFPAT_COPY_TTL_OUT:
        case of13::OFPAT_COPY_TTL_IN:
        case of13::OFPAT_SET_MPLS_TTL:
        case of13::OFPAT_DEC_MPLS_TTL:
        case of13::OFPAT_PUSH_VLAN:
        case of13::OFPAT_POP_VLAN:
        case of13::OFPAT_PUSH_MPLS:
        case of13::OFPAT_POP_MPLS:
        case of13::OFPAT_SET_QUEUE:
            std::cerr << "Unsupported apply action type" << std::endl;
            break;
        case of13::OFPAT_GROUP:
            break;
        case of13::OFPAT_SET_NW_TTL:
        case of13::OFPAT_DEC_NW_TTL:
            std::cerr << "Unsupported apply action type" << std::endl;
            break;
        case of13::OFPAT_SET_FIELD:
            break;
        case of13::OFPAT_PUSH_PBB:
        case of13::OFPAT_POP_PBB:
        case of13::OFPAT_EXPERIMENTER:
        default:
            std::cerr << "Unsupported apply action type" << std::endl;
            break;
        }
    }
    return std::move(actions);
}

of13::InstructionSet Parser::get_instructions(Actions actions) const
{
    of13::InstructionSet instructions;
    return std::move(instructions);
}
