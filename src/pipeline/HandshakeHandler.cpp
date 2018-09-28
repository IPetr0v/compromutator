#include "HandshakeHandler.hpp"

namespace pipeline {

using namespace fluid_msg;

Action HandshakeHandler::visit(of13::Hello&)
{
    return Action::FORWARD;
}

Action HandshakeHandler::visit(of13::Error&)
{
    return Action::FORWARD;
}

Action HandshakeHandler::visit(of13::EchoRequest&)
{
    return Action::FORWARD;
}

Action HandshakeHandler::visit(of13::EchoReply&)
{
    return Action::FORWARD;
}

Action HandshakeHandler::visit(of13::FeaturesRequest&)
{
    return Action::FORWARD;
}

Action HandshakeHandler::visit(of13::FeaturesReply& features_reply)
{
    info_.id = features_reply.datapath_id();
    info_.table_number = features_reply.n_tables();
    info_status_.features = true;
    try_establish();
    return Action::FORWARD;
}

Action HandshakeHandler::visit(of13::MultipartRequestPortDescription&)
{
    return Action::FORWARD;
}

Action HandshakeHandler::visit(of13::MultipartReplyPortDescription& port_desc)
{
    for (const auto &port : port_desc.ports()) {
        info_.ports.emplace_back(port.port_no(), port.curr_speed());
    }
    info_status_.ports = true;
    try_establish();
    return Action::FORWARD;
}

Action HandshakeHandler::visit(OFMsg&)
{
    return Action::ENQUEUE;
}

void HandshakeHandler::try_establish()
{
    if (not is_established_ && info_status_.isFull()) {
        switch_manager_.addSwitch(connection_id_, std::move(info_));
        rule_manager_.initSwitch(info_.id);
        is_established_ = true;
    }
}

} // namespace pipeline
