#include "Parser.hpp"

using namespace proto;

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
            // Add DROP action if there are other actions
            actions_.port_actions.emplace_back(PortActionBase::dropAction());
        }
        return std::move(actions_);
    }

    void addTransfer(Transfer&& transfer) {
        latest_transfer_ *= std::move(transfer);
    }

    void addPortAction(PortId port_id) {
        switch (port_id) {
        case SpecialPort::NONE:
        case SpecialPort::ANY:
            std::cerr << "Parser error: Wrong port ID" << std::endl;
            break;
        case SpecialPort::ALL:
            std::cerr << "Parser error: Unsupported ALL action" << std::endl;
            break;
        case SpecialPort::CONTROLLER:
            actions_.port_actions.emplace_back(PortActionBase::controllerAction());
            break;
        default:
            actions_.port_actions.emplace_back(port_id);
            break;
        }
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

of13::FlowMod Parser::getFlowMod(RuleInfoPtr rule)
{
    of13::FlowMod flow_mod;

    flow_mod.table_id(rule->table_id);
    flow_mod.priority(rule->priority);
    flow_mod.cookie(rule->cookie);
    flow_mod.match(get_of_match(rule->match));
    auto of_instr = get_of_instructions(rule->actions);
    flow_mod.instructions(of_instr);

    return flow_mod;
}

of13::FlowStats Parser::getFlowStats(RuleInfoPtr rule, RuleStatsFields stats)
{
    of13::FlowStats flow_stats;

    flow_stats.table_id(rule->table_id);
    // TODO: Implement duration
    flow_stats.duration_sec();
    flow_stats.duration_nsec();
    flow_stats.priority(rule->priority);
    flow_stats.cookie(rule->cookie);
    flow_stats.match(get_of_match(rule->match));
    flow_stats.packet_count(stats.packet_count);
    flow_stats.byte_count(stats.byte_count);

    return flow_stats;
}

of13::MultipartReplyFlow Parser::getMultipartReplyFlow(RuleReplyPtr reply)
{
    of13::MultipartReplyFlow reply_flow;

    reply_flow.flags(0);
    for (const auto& flow : reply->flows) {
        reply_flow.add_flow_stats(getFlowStats(flow.rule, flow.stats));
    }

    return reply_flow;
}

of13::MultipartRequestFlow Parser::getMultipartRequestFlow(RuleInfoPtr rule)
{
    of13::MultipartRequestFlow request_flow;

    request_flow.flags(0);
    request_flow.table_id(rule->table_id);//of13::OFPTT_ALL
    request_flow.out_port(of13::OFPP_ANY);
    request_flow.out_group(of13::OFPG_ANY);
    request_flow.cookie(rule->cookie);
    request_flow.cookie_mask((uint64_t)-1);
    request_flow.match(get_of_match(rule->match));

    return request_flow;
}

Match Parser::get_match(of13::Match match)
{
    PortId in_port = SpecialPort::ANY;
    auto bit_vector = BitVectorBridge(
        BitMask::wholeSpace(Mapping::HEADER_SIZE)
    );

    // Input port
    if (match.in_port()) {
        in_port = match.in_port()->value();
    }

    // Ethernet
    bit_vector.setField<Mapping::EthSrc>(match.eth_src());
    bit_vector.setField<Mapping::EthDst>(match.eth_dst());
    bit_vector.setField<Mapping::EthType>(match.eth_type());

    // IPv4
    bit_vector.setField<Mapping::IPProto>(match.ip_proto());
    bit_vector.setField<Mapping::IPv4Src>(match.ipv4_src());
    bit_vector.setField<Mapping::IPv4Dst>(match.ipv4_dst());

    // TCP
    bit_vector.setField<Mapping::TCPSrc>(match.tcp_src());
    bit_vector.setField<Mapping::TCPDst>(match.tcp_dst());

    // UDP
    bit_vector.setField<Mapping::UDPSrc>(match.udp_src());
    bit_vector.setField<Mapping::UDPDst>(match.udp_dst());

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
    of_match.add_oxm_field(bit_vector.getField<Mapping::EthSrc>());
    of_match.add_oxm_field(bit_vector.getField<Mapping::EthDst>());
    of_match.add_oxm_field(bit_vector.getField<Mapping::EthType>());

    if (of_match.eth_type()) {
        switch (of_match.eth_type()->value()) {
        case Ethernet::TYPE::IPv4: {
            of_match.add_oxm_field(bit_vector.getField<Mapping::IPv4Src>());
            of_match.add_oxm_field(bit_vector.getField<Mapping::IPv4Dst>());
            of_match.add_oxm_field(bit_vector.getField<Mapping::IPProto>());

            if (of_match.ip_proto()) {
                if (IPv4::PROTO::TCP == of_match.ip_proto()->value()) {
                    of_match.add_oxm_field(bit_vector.getField<Mapping::TCPSrc>());
                    of_match.add_oxm_field(bit_vector.getField<Mapping::TCPDst>());
                }
                else if (IPv4::PROTO::UDP == of_match.ip_proto()->value()) {
                    of_match.add_oxm_field(bit_vector.getField<Mapping::UDPSrc>());
                    of_match.add_oxm_field(bit_vector.getField<Mapping::UDPDst>());
                }
            }
            break;
        }
        case Ethernet::TYPE::ARP: {
            break;
        }
        case Ethernet::TYPE::IPv6: {
            break;
        }
        default:
            break;
        }
    }

    return of_match;
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
    return actions_bridge;
}

Transfer Parser::get_transfer(const of13::SetFieldAction* action)
{
    PortId in_port = SpecialPort::ANY;
    PortId out_port = SpecialPort::NONE;
    auto bit_vector = BitVectorBridge(
        BitMask::wholeSpace(Mapping::HEADER_SIZE)
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

of13::InstructionSet Parser::get_of_instructions(ActionsBase actions)
{
    of13::InstructionSet instructions;
    // TODO: implement instruction creation
    for (const auto& port_action : actions.port_actions) {
        // TODO: rewrite action creation, implement general case
        // Output action
        of13::ApplyActions actions;
        of13::OutputAction* output_action;
        switch (port_action.port_id) {
        case SpecialPort::NONE:
            std::cerr << "Parser error: Wrong port ID" << std::endl;
        case SpecialPort::ANY:
            output_action = new of13::OutputAction(
                of13::OFPP_ANY, of13::OFPCML_NO_BUFFER);
            break;
        case SpecialPort::ALL:
            output_action = new of13::OutputAction(
                of13::OFPP_ALL, of13::OFPCML_NO_BUFFER);
            break;
        case SpecialPort::CONTROLLER:
            output_action = new of13::OutputAction(
                of13::OFPP_CONTROLLER, of13::OFPCML_NO_BUFFER);
            break;
        default:
            output_action = new of13::OutputAction(
                port_action.port_id, of13::OFPCML_NO_BUFFER);
            break;
        }
        actions.add_action(output_action);
        instructions.add_instruction(actions);

        // Set field
        //port_action.transfer;
    }
    for (const auto& table_action : actions.table_actions) {
        of13::GoToTable goto_table(table_action.table_id);
        instructions.add_instruction(goto_table);
    }
    return instructions;
}
