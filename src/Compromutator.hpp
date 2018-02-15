#pragma once

#include "openflow/Types.hpp"
#include "openflow/Parser.hpp"
#include "ConcurrencyPrimitives.hpp"
#include "proxy/Event.hpp"
#include "proxy/Proxy.hpp"
#include "NetworkSpace.hpp"
#include "Detector.hpp"

#include <fluid/ofcommon/msg.hh>
#include <fluid/of10msg.hh>
#include <fluid/of13msg.hh>
#include <fluid/of10/openflow-10.h>
#include <fluid/of13/openflow-13.h>

#include <atomic>

class Compromutator
{
public:
    explicit Compromutator(ProxySettings settings);
    ~Compromutator();

    void run();

private:
    std::atomic_bool is_running_;
    std::shared_ptr<Alarm> alarm_;
    Proxy proxy_;
    Parser parser_;
    Detector detector_;

    void on_timeout();
    /// TODO: consider changing a name - should include "instruction"
    void on_detector_event();
    void on_proxy_event();
    void on_controller_message(ConnectionId connection_id, Message message);
    void on_switch_message(ConnectionId connection_id, Message message);

    void on_flow_mod(ConnectionId connection_id, Message message);

};
