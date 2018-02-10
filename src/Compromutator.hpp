#pragma once

#include "openflow/Types.hpp"
#include "ConcurrentQueue.hpp"
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
    explicit Compromutator(ProxySettings settings):
        alarm_(std::make_shared<Alarm>()), proxy_(settings, alarm_) {}

    void run();

private:
    std::shared_ptr<Alarm> alarm_;
    Proxy proxy_;

    std::atomic_bool is_running_;

    void on_timeout();
    void on_proxy_event();
    void on_detector_event();
    void on_controller_message(ConnectionId connection_id, Message message);
    void on_switch_message(ConnectionId connection_id, Message message);

};
