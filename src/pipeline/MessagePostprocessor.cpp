#include "MessagePostprocessor.hpp"

namespace pipeline {

using namespace fluid_msg;

void MessagePostprocessor::visit(fluid_msg::of13::FlowMod& flow_mod)
{
    ctrl_.performance_monitor.finishMeasurement(connection_id_, flow_mod.xid());
}

void MessagePostprocessor::visit(OFMsg&)
{

}

} // namespace pipeline
