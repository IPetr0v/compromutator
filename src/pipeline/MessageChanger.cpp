#include "MessageChanger.hpp"

namespace pipeline {

using namespace fluid_msg;

RawMessage MessageChanger::visit(of13::FlowMod& flow_mod)
{
    // TODO: uncomment table changing
    //flow_mod.table_id(flow_mod.table_id() + 1);
    return {flow_mod};
}

RawMessage MessageChanger::visit(of13::MultipartRequestFlow& request_flow)
{
    // TODO: uncomment table changing
    //request_flow.table_id(request_flow.table_id() + 1);
    return {request_flow};
}

RawMessage MessageChanger::visit(of13::MultipartReplyFlow& reply_flow)
{
    // TODO: uncomment table changing
    //for (auto& flow_stats : reply_flow.flow_stats()) {
    //    flow_stats.table_id(flow_stats.table_id() - 1);
    //}
    return {reply_flow};
}

RawMessage MessageChanger::visit(OFMsg& message)
{
    return {message};
}

} // namespace pipeline
