#pragma once

#include "Proto.hpp"
#include "Types.hpp"
#include "Detector.hpp"
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

class XidManager
{
public:
    XidManager(): last_id_(1u) {}
    // TODO: CRITICAL - xid from the real controller may be equal to detectors!
    uint32_t getXid() {
        // TODO: implement
        return last_id_++;
    }
    void deleteXid(uint32_t xid) {

    }
private:
    uint32_t last_id_;
};

class SwitchManager
{
public:
    explicit SwitchManager(Detector& detector): detector_(detector) {}

    const SwitchInfo* getSwitch(ConnectionId connection_id) const;
    void addSwitch(ConnectionId connection_id, SwitchInfo&& info);
    void deleteSwitch(ConnectionId connection_id);

    std::pair<ConnectionId, bool> getConnectionId(SwitchId switch_id) const;

private:
    Detector& detector_;

    std::unordered_map<SwitchId, ConnectionId> connection_map_;
    std::unordered_map<ConnectionId, SwitchInfo> switch_map_;
};

class LinkDiscovery
{
public:
    explicit LinkDiscovery(SwitchManager& switch_manager, Detector& detector):
        switch_manager_(switch_manager), detector_(detector) {}

    void handleLLDP(ConnectionId connection_id, PortId in_port,
                    const proto::LLDP& lldp);

private:
    SwitchManager& switch_manager_;
    Detector& detector_;

    SwitchId get_switch_id(const proto::LLDP& lldp) const;
    PortId get_port_id(const proto::LLDP& lldp) const;
};

class RuleManager
{
public:
    RuleManager(XidManager& xid_manager, SwitchManager& switch_manager,
                Sender sender):
        xid_manager_(xid_manager), switch_manager_(switch_manager),
        sender_(sender) {}

    void installRule(const RuleInfo& info);
    void deleteRule(const RuleInfo& info);
    void initSwitch(SwitchId id);

private:
    XidManager& xid_manager_;
    SwitchManager& switch_manager_;
    Sender sender_;

    void send_flow_mod(SwitchId switch_id, fluid_msg::of13::FlowMod flow_mod);
};

class StatsQuerier
{
public:
    StatsQuerier(XidManager& xid_manager, SwitchManager& switch_manager,
                 Sender sender):
        xid_manager_(xid_manager), switch_manager_(switch_manager),
        sender_(sender) {}

    //void getPortDesc(ConnectionId id);
    void getRuleStats(RequestId request_id, const RuleInfo& info);
    void getPortStats(RequestId request_id, SwitchId switch_id, PortId port_id);

    // TODO: consider using std::optional
    std::pair<RequestId, bool> popRequestId(uint32_t xid);

private:
    XidManager& xid_manager_;
    SwitchManager& switch_manager_;
    Sender sender_;

    std::map<uint32_t, RequestId> request_id_map_;

};

struct Controller
{
    Controller(std::shared_ptr<Alarm> alarm, Sender sender);

    Detector detector;
    XidManager xid_manager;
    SwitchManager switch_manager;
    LinkDiscovery link_discovery;
    RuleManager rule_manager;
    StatsQuerier stats_manager;
};
