#include "Compromutator.hpp"

#include <chrono>

Compromutator::Compromutator(ProxySettings settings):
    is_running_(false),
    alarm_(std::make_shared<Alarm>()),
    proxy_(settings, alarm_),
    controller_(alarm_, proxy_.getSender()),
    pipeline_(proxy_.getSender(), controller_)
{

}

Compromutator::~Compromutator()
{
    is_running_ = false;
}

void Compromutator::run()
{
    is_running_ = true;
    while (is_running_) {
        auto status = alarm_->wait(std::chrono::milliseconds(100));
        if (status == Alarm::Status::TIMEOUT) {
            controller_.detector.prepareInstructions();
            pipeline_.addBarrier();

            // TODO: delete this after implementing instruction handler
            pipeline_.flushPipeline();
        }
        else {
            if (controller_.detector.instructionsExist()) {
                handle_detector_instruction();
                pipeline_.flushPipeline();
            }
            if (proxy_.eventsExist()) {
                handle_proxy_event();
            }
        }
    }
}

void Compromutator::handle_detector_instruction()
{
    // TODO: implement instruction handling
    while (controller_.detector.instructionsExist()) {
        auto instruction = controller_.detector.getInstruction();
        /*for (const auto& request : instruction.requests.data) {
            auto request_id = request->id;
            if (auto rule_request = RuleRequest::pointerCast(request)) {
                auto rule = rule_request->rule;
                pending_requests_.emplace(request_id, rule_request);
                controller_.getRuleStats(request_id, rule);
            }
            else if (auto port_request = PortRequest::pointerCast(request)) {
                auto port = port_request->port;
                pending_requests_.emplace(request_id, port_request);
                controller_.getPortStats(request_id, port);
            }
            else {
                assert(0);
            }
        }*/

        auto rules_to_delete = instruction.interceptor_diff.getRulesToDelete();
        for (const auto& rule_to_delete : rules_to_delete) {
            //std::cout<<"Delete: "<<rule_to_delete<<std::endl;
            controller_.rule_manager.deleteRule(rule_to_delete);
        }

        auto rules_to_add = instruction.interceptor_diff.getRulesToAdd();
        for (const auto& rule_to_add : rules_to_add) {
            //std::cout<<"Add: "<<rule_to_add<<std::endl;
            controller_.rule_manager.installRule(rule_to_add);
        }
    }
}

void Compromutator::handle_proxy_event()
{
    while (proxy_.eventsExist()) {
        auto event = proxy_.getEvent();
        switch (event->type) {
        case EventType::CONNECTION: {
            auto connection_event = ConnectionEvent::pointerCast(event);
            if (connection_event->type == ConnectionEventType::NEW) {
                pipeline_.addConnection(connection_event->id);
            }
            else {
                pipeline_.deleteConnection(connection_event->id);
            }
            break;
        }
        case EventType::MESSAGE: {
            auto message_event = MessageEvent::pointerCast(event);
            pipeline_.processMessage(message_event->message);
            break;
        }
        }
    }
}
