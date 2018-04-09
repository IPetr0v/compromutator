#include "Parser.hpp"

class Parser::ActionsBaseBridge
{
public:
    ActionsBaseBridge():
        actions_(ActionsBase()),
        latest_transfer_(Transfer::identityTransfer()) {}
    explicit ActionsBaseBridge(ActionsBase&& actions):
        actions_(std::move(actions)),
        latest_transfer_(Transfer::identityTransfer()) {}

    ActionsBase popActionsBase() {
        if (actions_.empty()) {
            this->addDropAction();
        }
        return std::move(actions_);
    }

    void addTransfer(Transfer&& transfer) {
        latest_transfer_ *= std::move(transfer);
    }

    void addPortAction(PortId port_id) {
        actions_.port_actions.emplace_back(port_id);
    }
    void addDropAction() {
        actions_.port_actions.emplace_back(PortActionBase::dropAction());
    }
    void addControllerAction() {
        actions_.port_actions.emplace_back(PortActionBase::controllerAction());
    }
    void addGroupAction(GroupId group_id) {
        actions_.group_actions.emplace_back(latest_transfer_, group_id);
    }
    void addTableAction(TableId table_id) {
        // TODO: table action should be single
        actions_.table_actions.emplace_back(latest_transfer_, table_id);
    }

    void operator+=(ActionsBaseBridge&& other) {
        actions_ += std::move(other.actions_);
        latest_transfer_ *= std::move(other.latest_transfer_);
    }

private:
    ActionsBase actions_;
    Transfer latest_transfer_;
};

of13::FlowMod Parser::getFlowMod(RuleInfo rule)
{
    of13::FlowMod flow_mod;

    flow_mod.table_id(rule.table_id);
    flow_mod.priority(rule.priority);
    flow_mod.match(get_of_match(rule.match));
    flow_mod.instructions(get_instructions(rule.actions));

    return std::move(flow_mod);
}

of13::MultipartRequestFlow Parser::getMultipartRequestFlow(RuleInfo rule)
{
    of13::MultipartRequestFlow request_flow;

    request_flow.table_id(rule.table_id);
    request_flow.match(get_of_match(rule.match));

    return std::move(request_flow);
}

Match Parser::get_match(of13::Match match)
{
    PortId in_port = SpecialPort::ANY;
    auto bit_vector = BitVectorBridge(
        BitVector::wholeSpace(Mapping::HEADER_SIZE)
    );

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

    return Match(in_port, bit_vector.popBitVector());
}

of13::Match Parser::get_of_match(const Match& match)
{
    of13::Match of_match;
    auto bit_vector = BitVectorBridge(match.header());

    // Input port
    if (SpecialPort::ANY != match.inPort()) {
        of_match.add_oxm_field(new of13::InPort(match.inPort()));
    }

    // Ethernet
    auto eth_src = bit_vector.getField<Mapping::EthSrc>();
    if (eth_src) of_match.add_oxm_field(eth_src);

    auto eth_dst = bit_vector.getField<Mapping::EthDst>();
    if (eth_dst) of_match.add_oxm_field(eth_dst);

    auto eth_type = bit_vector.getField<Mapping::EthType>();
    if (eth_type) of_match.add_oxm_field(eth_type);

    if (ETH_TYPE::IPv4 == eth_type->value()) {
        // IPv4
        auto ip_proto = bit_vector.getField<Mapping::IPProto>();
        if (ip_proto) of_match.add_oxm_field(ip_proto);

        auto ipv4_src = bit_vector.getField<Mapping::IPv4Src>();
        if (ipv4_src) of_match.add_oxm_field(ipv4_src);

        auto ipv4_dst = bit_vector.getField<Mapping::IPv4Dst>();
        if (ipv4_dst) of_match.add_oxm_field(ipv4_dst);

        if (IP_PROTO::TCP == ip_proto->value()) {
            // TCP
            auto tcp_src = bit_vector.getField<Mapping::TCPSrc>();
            if (tcp_src) of_match.add_oxm_field(tcp_src);

            auto tcp_dst = bit_vector.getField<Mapping::TCPDst>();
            if (tcp_dst) of_match.add_oxm_field(tcp_dst);
        }
        else if (IP_PROTO::UDP == ip_proto->value()) {
            // UDP
            auto udp_src = bit_vector.getField<Mapping::UDPSrc>();
            if (udp_src) of_match.add_oxm_field(udp_src);

            auto udp_dst = bit_vector.getField<Mapping::UDPDst>();
            if (udp_dst) of_match.add_oxm_field(udp_dst);
        }
    }

    return std::move(of_match);
}

ActionsBase Parser::get_actions(of13::InstructionSet instructions)
{
    ActionsBaseBridge actions_bridge;
    for (auto instruction : instructions.instruction_set()) {
        switch (instruction->type()) {
        case of13::OFPIT_GOTO_TABLE: {
            auto goto_table = dynamic_cast<of13::GoToTable*>(instruction);
            actions_bridge.addTableAction(goto_table->table_id());
            break;
        }
        case of13::OFPIT_WRITE_METADATA:
        case of13::OFPIT_WRITE_ACTIONS:
            std::cerr << "Unsupported instruction type" << std::endl;
            break;
        case of13::OFPIT_APPLY_ACTIONS: {
            auto apply_actions = dynamic_cast<of13::ApplyActions*>(instruction);
            actions_bridge += get_apply_actions(apply_actions);
            break;
        }
        case of13::OFPIT_CLEAR_ACTIONS:
        case of13::OFPIT_METER:
        case of13::OFPIT_EXPERIMENTER:
            std::cerr << "Unsupported instruction type" << std::endl;
            break;
        default:
            std::cerr << "Unknown instruction type" << std::endl;
            break;
        }
    }
    return actions_bridge.popActionsBase();
}

Parser::ActionsBaseBridge
Parser::get_apply_actions(of13::ApplyActions* actions)
{
    ActionsBaseBridge actions_bridge;
    for (auto action : actions->actions().action_list()) {
        switch (action->type()) {
        case of13::OFPAT_OUTPUT: {
            auto output_action = dynamic_cast<of13::OutputAction*>(action);
            assert(output_action);
            actions_bridge.addPortAction(output_action->port());
            break;
        }
        case of13::OFPAT_GROUP: {
            auto group_action = dynamic_cast<of13::GroupAction*>(action);
            assert(group_action);
            actions_bridge.addGroupAction(group_action->group_id());
            break;
        }
        case of13::OFPAT_SET_FIELD: {
            auto set_field_action = dynamic_cast<of13::SetFieldAction*>(action);
            assert(set_field_action);
            actions_bridge.addTransfer(get_transfer(set_field_action));
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
        case of13::OFPAT_SET_NW_TTL:
        case of13::OFPAT_DEC_NW_TTL:
        case of13::OFPAT_PUSH_PBB:
        case of13::OFPAT_POP_PBB:
        case of13::OFPAT_EXPERIMENTER:
        default:
            std::cerr << "Unsupported apply action type" << std::endl;
            break;
        }
    }
    return std::move(actions_bridge);
}

Transfer Parser::get_transfer(const of13::SetFieldAction* action)
{
    PortId in_port = SpecialPort::ANY;
    PortId out_port = SpecialPort::NONE;
    auto bit_vector = BitVectorBridge(
        BitVector::wholeSpace(Mapping::HEADER_SIZE)
    );

    auto field = action->field();

    // Input port
    auto in_port_field = dynamic_cast<const of13::InPort*>(field);
    if (in_port_field) {
        in_port = in_port_field->value();
    }

    // Ethernet
    auto eth_src = dynamic_cast<const of13::EthSrc*>(field);
    if (eth_src) bit_vector.setField<Mapping::EthSrc>(eth_src);

    auto eth_dst = dynamic_cast<const of13::EthDst*>(field);
    if (eth_dst) bit_vector.setField<Mapping::EthDst>(eth_dst);

    auto eth_type = dynamic_cast<const of13::EthType*>(field);
    if (eth_type) bit_vector.setField<Mapping::EthType>(eth_type);

    // IPv4
    auto ip_proto = dynamic_cast<const of13::IPProto*>(field);
    if (ip_proto) bit_vector.setField<Mapping::IPProto>(ip_proto);

    auto ipv4_src = dynamic_cast<const of13::IPv4Src*>(field);
    if (ipv4_src) bit_vector.setField<Mapping::IPv4Src>(ipv4_src);

    auto ipv4_dst = dynamic_cast<const of13::IPv4Dst*>(field);
    if (ipv4_dst) bit_vector.setField<Mapping::IPv4Dst>(ipv4_dst);

    // TCP
    auto tcp_src = dynamic_cast<const of13::TCPSrc*>(field);
    if (tcp_src) bit_vector.setField<Mapping::TCPSrc>(tcp_src);

    auto tcp_dst = dynamic_cast<const of13::TCPDst*>(field);
    if (tcp_dst) bit_vector.setField<Mapping::TCPDst>(tcp_dst);

    // UDP
    auto udp_src = dynamic_cast<const of13::UDPSrc*>(field);
    if (udp_src) bit_vector.setField<Mapping::UDPSrc>(udp_src);

    auto udp_dst = dynamic_cast<const of13::UDPDst*>(field);
    if (udp_dst) bit_vector.setField<Mapping::UDPDst>(udp_dst);

    return Transfer(in_port, out_port,
                    HeaderChanger(bit_vector.popBitVector()));
}

of13::InstructionSet Parser::get_instructions(ActionsBase actions)
{
    of13::InstructionSet instructions;
    return std::move(instructions);
}
