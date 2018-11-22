#pragma once

#include "Types.hpp"
#include "ConcurrencyPrimitives.hpp"
#include "proxy/Event.hpp"
#include "proxy/Proxy.hpp"
#include "pipeline/Pipeline.hpp"
#include "NetworkSpace.hpp"
#include "Detector.hpp"
#include "Controller.hpp"

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
    explicit Compromutator(ProxySettings settings,
                           uint32_t timeout_duration,
                           std::string measurement_filename);
    ~Compromutator();

    void run();

private:
    std::atomic_bool is_running_;
    std::shared_ptr<Alarm> alarm_;

    Proxy proxy_;
    Controller controller_;
    pipeline::Pipeline pipeline_;

    void handle_detector_instruction();
    void handle_proxy_event();
};
