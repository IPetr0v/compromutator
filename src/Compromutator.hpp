#pragma once

#include "Types.hpp"
#include "ConcurrencyPrimitives.hpp"
#include "proxy/Event.hpp"
#include "proxy/Proxy.hpp"
#include "MessagePipeline.hpp"
#include "NetworkSpace.hpp"
#include "Detector.hpp"

#include <fluid/ofcommon/msg.hh>
#include <fluid/of10msg.hh>
#include <fluid/of13msg.hh>
#include <fluid/of10/openflow-10.h>
#include <fluid/of13/openflow-13.h>

#include <atomic>
#include <memory>

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
    Detector detector_;
    Controller controller_;
    pipeline::MessagePipeline message_pipeline_;

    void handle_detector_instruction();
    void handle_proxy_event();
};
