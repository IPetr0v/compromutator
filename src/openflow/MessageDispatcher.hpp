#pragma once

#include "../proxy/Event.hpp"

#include <fluid/ofcommon/msg.hh>
#include <fluid/of10msg.hh>
#include <fluid/of13msg.hh>
#include <fluid/of10/openflow-10.h>
#include <fluid/of13/openflow-13.h>

#include <type_traits>

template<class ReturnType, class BaseMessage>
class MessageDispatcher
{
public:
    template<class Message>
    struct Visitor;

    struct VisitorInterface {
        template<class Message>
        ReturnType visitMessage(Message& message) {
            if (auto self = dynamic_cast<Visitor<Message>*>(this)) {
                return self->visit(message);
            }
            else if (auto self = dynamic_cast<Visitor<BaseMessage>*>(this)) {
                return self->visit(message);
            }
            else {
                throw std::invalid_argument("Visitor error: Wrong visitor");
            }
        }

        virtual ~VisitorInterface() = default;
    };

    // TODO: add Args...
    template<class Message>
    struct Visitor : public virtual VisitorInterface {
        virtual ReturnType visit(Message& message) = 0;
    };

    struct DispatchableInterface {
        virtual ReturnType accept(VisitorInterface& visitor) = 0;
    };

    template<class Message>
    struct DispatchableMessage : virtual Message, DispatchableInterface {
        ReturnType accept(VisitorInterface& visitor) override {
            return visitor.visitMessage((Message&)*this);
        }
    };

    ReturnType operator()(RawMessage raw_message, VisitorInterface& visitor) {
        return dispatch_message(raw_message, visitor);
    }

private:
    template<class Message>
    ReturnType dispatch(RawMessage raw_message,
                        VisitorInterface& visitor) const {
        auto message = std::make_unique<DispatchableMessage<Message>>();
        auto error = message->unpack(raw_message.data());
        if (error) {
            throw std::logic_error(
                "Message unpack error: Error code " + std::to_string(error)
            );
        }
        return message->accept(visitor);
    }

    ReturnType dispatch_message(RawMessage raw_message,
                                VisitorInterface& visitor) const {
        using namespace fluid_msg;

        switch (raw_message.type) {
        // Symmetric messages
        case of13::OFPT_HELLO:
            return dispatch<of13::Hello>(raw_message, visitor);
        case of13::OFPT_ERROR:
            return dispatch<of13::Error>(raw_message, visitor);
        case of13::OFPT_ECHO_REQUEST:
            return dispatch<of13::EchoRequest>(raw_message, visitor);
        case of13::OFPT_ECHO_REPLY:
            return dispatch<of13::EchoReply>(raw_message, visitor);
        case of13::OFPT_EXPERIMENTER:
            return dispatch<of13::Experimenter>(raw_message, visitor);

        // Controller messages
        case of13::OFPT_FEATURES_REQUEST:
            return dispatch<of13::FeaturesRequest>(raw_message, visitor);
        case of13::OFPT_GET_CONFIG_REQUEST:
            return dispatch<of13::GetConfigRequest>(raw_message, visitor);
        case of13::OFPT_SET_CONFIG:
            return dispatch<of13::SetConfig>(raw_message, visitor);
        case of13::OFPT_PACKET_OUT:
            return dispatch<of13::PacketOut>(raw_message, visitor);
        case of13::OFPT_FLOW_MOD:
            return dispatch<of13::FlowMod>(raw_message, visitor);
        case of13::OFPT_GROUP_MOD:
            return dispatch<of13::GroupMod>(raw_message, visitor);
        case of13::OFPT_PORT_MOD:
            return dispatch<of13::PortMod>(raw_message, visitor);
        case of13::OFPT_TABLE_MOD:
            return dispatch<of13::TableMod>(raw_message, visitor);
        case of13::OFPT_METER_MOD:
            return dispatch<of13::MeterMod>(raw_message, visitor);
        case of13::OFPT_BARRIER_REQUEST:
            return dispatch<of13::BarrierRequest>(raw_message, visitor);
        case of13::OFPT_QUEUE_GET_CONFIG_REQUEST:
            return dispatch<of13::QueueGetConfigRequest>(raw_message, visitor);
        case of13::OFPT_ROLE_REQUEST:
            return dispatch<of13::RoleRequest>(raw_message, visitor);
        case of13::OFPT_GET_ASYNC_REQUEST:
            return dispatch<of13::GetAsyncRequest>(raw_message, visitor);
        case of13::OFPT_SET_ASYNC:
            return dispatch<of13::SetAsync>(raw_message, visitor);
        case of13::OFPT_MULTIPART_REQUEST:
            return dispatch_multipart<of13::MultipartRequest>(raw_message, visitor);

        // Switch messages
        case of13::OFPT_FEATURES_REPLY:
            return dispatch<of13::FeaturesReply>(raw_message, visitor);
        case of13::OFPT_GET_CONFIG_REPLY:
            return dispatch<of13::GetConfigReply>(raw_message, visitor);
        case of13::OFPT_PACKET_IN:
            return dispatch<of13::PacketIn>(raw_message, visitor);
        case of13::OFPT_FLOW_REMOVED:
            return dispatch<of13::FlowRemoved>(raw_message, visitor);
        case of13::OFPT_PORT_STATUS:
            return dispatch<of13::PortStatus>(raw_message, visitor);
        case of13::OFPT_BARRIER_REPLY:
            return dispatch<of13::BarrierReply>(raw_message, visitor);
        case of13::OFPT_QUEUE_GET_CONFIG_REPLY:
            return dispatch<of13::QueueGetConfigReply>(raw_message, visitor);
        case of13::OFPT_ROLE_REPLY:
            return dispatch<of13::RoleReply>(raw_message, visitor);
        case of13::OFPT_GET_ASYNC_REPLY:
            return dispatch<of13::GetAsyncReply>(raw_message, visitor);
        case of13::OFPT_MULTIPART_REPLY:
            return dispatch_multipart<of13::MultipartReply>(raw_message, visitor);

        // Unknown message
        default:
            throw std::invalid_argument(
                "Dispatcher error: Unknown message type - " +
                std::to_string(raw_message.type)
            );
        }
    }

    template<class MultipartType>
    ReturnType dispatch_multipart(RawMessage raw_message,
                                  VisitorInterface& visitor) const {
        using namespace fluid_msg;

        using IsRequest = std::is_same<MultipartType, of13::MultipartRequest>;
        using IsReply = std::is_same<MultipartType, of13::MultipartReply>;
        static_assert(IsRequest::value or IsReply::value,
                      "Only multipart messages allowed");

        MultipartType multipart_message;
        multipart_message.unpack(raw_message.data());

        switch (multipart_message.mpart_type()) {
        case of13::OFPMP_DESC:
            return IsReply::value
                   ? dispatch<of13::MultipartReplyDesc>(raw_message, visitor)
                   : dispatch<of13::MultipartRequestDesc>(raw_message, visitor);
        case of13::OFPMP_FLOW:
            return IsReply::value
                   ? dispatch<of13::MultipartReplyFlow>(raw_message, visitor)
                   : dispatch<of13::MultipartRequestFlow>(raw_message, visitor);
        case of13::OFPMP_AGGREGATE:
            return IsReply::value
                   ? dispatch<of13::MultipartReplyAggregate>(raw_message, visitor)
                   : dispatch<of13::MultipartRequestAggregate>(raw_message, visitor);
        case of13::OFPMP_TABLE:
            return IsReply::value
                   ? dispatch<of13::MultipartReplyTable>(raw_message, visitor)
                   : dispatch<of13::MultipartRequestTable>(raw_message, visitor);
        case of13::OFPMP_PORT_STATS:
            return IsReply::value
                   ? dispatch<of13::MultipartReplyPortStats>(raw_message, visitor)
                   : dispatch<of13::MultipartRequestPortStats>(raw_message, visitor);
        case of13::OFPMP_QUEUE:
            return IsReply::value
                   ? dispatch<of13::MultipartReplyQueue>(raw_message, visitor)
                   : dispatch<of13::MultipartRequestQueue>(raw_message, visitor);
        case of13::OFPMP_GROUP:
            return IsReply::value
                   ? dispatch<of13::MultipartReplyGroup>(raw_message, visitor)
                   : dispatch<of13::MultipartRequestGroup>(raw_message, visitor);
        case of13::OFPMP_GROUP_DESC:
            return IsReply::value
                   ? dispatch<of13::MultipartReplyGroupDesc>(raw_message, visitor)
                   : dispatch<of13::MultipartRequestGroupDesc>(raw_message, visitor);
        case of13::OFPMP_GROUP_FEATURES:
            return IsReply::value
                   ? dispatch<of13::MultipartReplyGroupFeatures>(raw_message, visitor)
                   : dispatch<of13::MultipartRequestGroupFeatures>(raw_message, visitor);
        case of13::OFPMP_METER:
            return IsReply::value
                   ? dispatch<of13::MultipartReplyMeter>(raw_message, visitor)
                   : dispatch<of13::MultipartRequestMeter>(raw_message, visitor);
        case of13::OFPMP_METER_CONFIG:
            return IsReply::value
                   ? dispatch<of13::MultipartReplyMeterConfig>(raw_message, visitor)
                   : dispatch<of13::MultipartRequestMeterConfig>(raw_message, visitor);
        case of13::OFPMP_METER_FEATURES:
            return IsReply::value
                   ? dispatch<of13::MultipartReplyMeterFeatures>(raw_message, visitor)
                   : dispatch<of13::MultipartRequestMeterFeatures>(raw_message, visitor);
        case of13::OFPMP_TABLE_FEATURES:
            return IsReply::value
                   ? dispatch<of13::MultipartReplyTableFeatures>(raw_message, visitor)
                   : dispatch<of13::MultipartRequestTableFeatures>(raw_message, visitor);
        case of13::OFPMP_PORT_DESC:
            return IsReply::value
                   ? dispatch<of13::MultipartReplyPortDescription>(raw_message, visitor)
                   : dispatch<of13::MultipartRequestPortDescription>(raw_message, visitor);
        case of13::OFPMP_EXPERIMENTER:
            return IsReply::value
                   ? dispatch<of13::MultipartReplyExperimenter>(raw_message, visitor)
                   : dispatch<of13::MultipartRequestExperimenter>(raw_message, visitor);
        default:
            throw std::invalid_argument(
                "Dispatcher error: Unknown multipart type - " +
                std::to_string(multipart_message.mpart_type())
            );
        }
    }
};
