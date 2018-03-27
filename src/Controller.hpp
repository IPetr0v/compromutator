#pragma once

#include "Types.hpp"
#include "network/Rule.hpp"
#include "network/Network.hpp"
#include "proxy/Proxy.hpp"
#include "openflow/MessageDispatcher.hpp"

#include <fluid/ofcommon/msg.hh>
#include <fluid/of10msg.hh>
#include <fluid/of13msg.hh>
#include <fluid/of10/openflow-10.h>
#include <fluid/of13/openflow-13.h>

#include <cstdint>

class Detector;

class XidFactory
{
public:
    uint32_t getXid() {
        // TODO: implement
        return 1u;
    }
    void deleteXid(uint32_t xid) {

    }

private:

};

// TODO: rename to ProxyController or not...
class Controller
{
public:
    // TODO: CRITICAL - xid from the real controller may be equal to detectors!

    Controller(Sender sender, Detector& detector);

    const SwitchInfo* getSwitch(ConnectionId connection_id) const;
    void addSwitch(ConnectionId connection_id, SwitchInfo&& info);
    void deleteSwitch(ConnectionId connection_id);

    //void getPortDesc(ConnectionId id);
    void getRuleStats(RequestId request_id, const RuleInfo& info);
    void getPortStats(RequestId request_id, SwitchId switch_id, PortId port_id);

    void installRule(const RuleInfo& info);
    void deleteRule(const RuleInfo& info);

    // TODO: consider using std::optional
    std::pair<RequestId, bool> popRequestId(uint32_t xid);

private:
    XidFactory xid_factory_;
    Sender sender_;
    Detector& detector_;

    std::unordered_map<SwitchId, ConnectionId> connection_map_;
    std::unordered_map<ConnectionId, SwitchInfo> switch_map_;

    std::map<uint32_t, RequestId> request_id_map_;

    std::pair<ConnectionId, bool> get_connection_id(SwitchId switch_id);
    // TODO: add const to add pack functions in fluid_msg
    void send_to_controller(ConnectionId id, fluid_msg::OFMsg& message);
    void send_to_switch(ConnectionId id, fluid_msg::OFMsg& message);

};
