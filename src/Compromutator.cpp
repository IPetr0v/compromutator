#include "Compromutator.hpp"

#include <chrono>

Compromutator::Compromutator(ProxySettings settings,
                             uint32_t timeout_duration,
                             std::string measurement_filename):
    is_running_(false),
    alarm_(std::make_shared<Alarm>(
        std::chrono::milliseconds(timeout_duration))),
    proxy_(settings, alarm_),
    controller_(alarm_, proxy_.getSender(), measurement_filename),
    pipeline_(proxy_.getSender(), controller_)
{
    std::cout<<"Timeout: "<<timeout_duration<<std::endl;
    std::cout<<"Measurements: "<<measurement_filename<<std::endl;
}

Compromutator::~Compromutator()
{
    is_running_ = false;
}

void Compromutator::run()
{
    is_running_ = true;
    while (is_running_) {
        //auto status = alarm_->wait(std::chrono::milliseconds(10));
        auto status = alarm_->wait();
        if (status == Alarm::Status::TIMEOUT) {
            pipeline_.addBarrier();
            controller_.detector.prepareInstructions();
        }
        else {
            if (controller_.detector.instructionsExist()) {
                pipeline_.flushPipeline();
                handle_detector_instruction();
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

        for (const auto& reply : instruction.replies) {
            controller_.stats_manager.sendRuleStats(reply);
        }

        for (const auto& request : instruction.requests.data) {
            auto request_id = request->id;
            if (auto rule_request = RuleRequest::pointerCast(request)) {
                controller_.stats_manager.getRuleStats(
                    request_id, rule_request->rule);
            }
            else {
                assert(0);
            }
        }

        std::set<SwitchId> affected_switches;
        auto& rules_to_delete = instruction.interceptor_diff.rules_to_delete;
        for (const auto& rule_to_delete : rules_to_delete) {
            affected_switches.insert(rule_to_delete->switch_id);
            controller_.rule_manager.deleteRulesByCookie(rule_to_delete);
        }
        for (const auto& switch_id: affected_switches) {
            controller_.rule_manager.sendBarrier(switch_id);
        }

        auto& rules_to_add = instruction.interceptor_diff.rules_to_add;
        for (const auto& rule_to_add : rules_to_add) {
            controller_.rule_manager.installRule(rule_to_add);
        }

        // DEBUG
        if (not instruction.replies.empty() or not rules_to_add.empty() or
            not rules_to_delete.empty()) {
            std::cout<<"--- REPLY: "<<instruction.replies.size()
                     <<" | REQUEST: "<<instruction.requests.data.size()
                     <<" | ADD_FLOW: "<<rules_to_add.size()
                     <<" | REMOVE_FLOW: "<<rules_to_delete.size()
                     <<std::endl;
            std::cout<<std::endl;
        }
        //DEBUG//for (const auto& rule_to_delete : instruction.interceptor_diff.rules_to_delete) {
        //DEBUG//    std::cout<<"------ Delete: "<<*rule_to_delete<<std::endl;
        //DEBUG//}
        //DEBUG//for (const auto& rule_to_add : instruction.interceptor_diff.rules_to_add) {
        //DEBUG//    std::cout<<"------ Add: "<<*rule_to_add<<std::endl;
        //DEBUG//}
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
