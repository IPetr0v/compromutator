#include "Compromutator.hpp"

#include <typeinfo>
#include <iostream>
#include <list>
#include <map>
#include <string.h>
#include <vector>
#include <unistd.h>

using namespace std;

class OpenFlowParser
{
    fluid_msg::of13::FlowMod parseFlowMod(Message message) const {
        assert(message.type == fluid_msg::of13::OFPT_FLOW_MOD);
        fluid_msg::of13::FlowMod flow_mod;
        flow_mod.unpack(message.data);
        return std::move(flow_mod);
    }
};

struct SwitchInfo
{
    SwitchId id;
    std::vector<PortId> ports;
    uint8_t table_number;
};

class TopologyTracker
{
public:
    void addConnection(ConnectionId id) {

    }
    /*SwitchInfo getNewSwitch(ConnectionId id,
                            fluid_msg::of13::FeaturesReply features_reply) {

    }*/

private:
    std::set<ConnectionId> empty_connections_;
    std::unordered_map<ConnectionId, SwitchId> switch_map_;
};

int main(int argc, char* argv[])
{
    ProxySettings proxy_settings;
    proxy_settings.controller_address = "127.0.0.1";
    proxy_settings.controller_port = 6655;
    proxy_settings.proxy_address = "127.0.0.1";
    proxy_settings.proxy_port = 6653;

    Compromutator compromutator(proxy_settings);

    compromutator.run();

    return 0;
}
