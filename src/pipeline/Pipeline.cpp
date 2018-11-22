#include "Pipeline.hpp"
#include "../Controller.hpp"

#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pipeline {

void Pipeline::addConnection(ConnectionId id)
{
    auto handshake_it = handshake_pipelines_.find(id);
    auto handler_it = message_handlers_.find(id);
    if (handshake_pipelines_.end() == handshake_it &&
        message_handlers_.end() == handler_it) {
        // Create message handler
        handshake_pipelines_.emplace(
            id, HandshakePipeline(id, controller_)
        );
        message_handlers_.emplace(
            id, MessageHandler(id, controller_)
        );
    }
    else {
        std::cerr << "Pipeline error: Already existing connection" << std::endl;
    }
    auto postprocessor_it = message_postprocessors_.find(id);
    if (message_postprocessors_.end() == postprocessor_it) {
        message_postprocessors_.emplace(
            id, MessagePostprocessor(id, controller_)
        );
    }
    else {
        std::cerr << "Pipeline error: Already existing connection" << std::endl;
    }
}

void Pipeline::deleteConnection(ConnectionId id)
{
    message_handlers_.erase(id);
}

void Pipeline::processMessage(Message message)
{
    std::lock_guard<std::mutex> lock(mutex_);
    // Check if the connection hasn't been established yet
    auto handshake_it = handshake_pipelines_.find(message.connection_id);
    if (handshake_pipelines_.end() != handshake_it) {
        try {
            // Dispatch message
            auto &pipeline = handshake_it->second;
            auto pre_action = Dispatcher()(message.raw_message,
                                           pipeline.handler);

            // Process message
            switch (pre_action) {
            case Action::FORWARD:
                handle_message(message);
                break;
            case Action::ENQUEUE:
                pipeline.queue.push(message);
                break;
            case Action::DROP:
                break;
            }

            // Check connection status
            if (pipeline.handler.established()) {
                // Handle queued messages
                while (not pipeline.queue.empty()) {
                    auto queued_message = pipeline.queue.front();
                    pipeline.queue.pop();
                    handle_message(queued_message);
                }

                // Delete handshake pipeline for the established connection
                handshake_pipelines_.erase(handshake_it);
            }
        }
        catch (const std::invalid_argument& error) {
            std::cerr << "Pipeline process error: " << error.what() << std::endl;
            handle_message(message);
        }
        catch (const std::logic_error& error) {
            std::cerr << "Pipeline process error: " << error.what() << std::endl;
            handle_message(message);
        }
    }
    else {
        handle_message(message);
    }
}

void Pipeline::addBarrier()
{
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.addBarrier();
}

void Pipeline::flushPipeline()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (not queue_.empty()) {
        auto messages = queue_.pop();
        //std::cout<<"Flush ("<<queue_.size()<<") -> "<<messages.size()<<std::endl;
        for (auto message : messages) {
            auto postprocessor_it = message_postprocessors_.find(
                message.connection_id
            );
            assert(message_postprocessors_.end() != postprocessor_it);
            auto& message_postprocessor = postprocessor_it->second;

            PostprocessorDispatcher()(message.raw_message,
                                      message_postprocessor);
            forward_message(message);
        }
        controller_.performance_monitor.flush();
    }
}

void Pipeline::handle_message(Message message)
{
    auto handler_it = message_handlers_.find(message.connection_id);
    // TODO: delete connections correctly
    assert(message_handlers_.end() != handler_it);
    auto& message_handler = handler_it->second;
    try {
        // Dispatch message
        auto action = Dispatcher()(message.raw_message, message_handler);

        // Change message
        auto new_message = Message(
            message.connection_id, message.origin,
            ChangerDispatcher()(message.raw_message, message_changer_)
        );

        // Process message
        switch (action) {
        case Action::FORWARD:
            forward_message(new_message);
            break;
        case Action::ENQUEUE:
            enqueue_message(new_message);
            break;
        case Action::DROP:
            break;
        }
    }
    catch (const std::invalid_argument& error) {
        std::cerr << "Pipeline process error: " << error.what() << std::endl;
        forward_message(message);
    }
    catch (const std::logic_error& error) {
        std::cerr << "Pipeline process error: " << error.what() << std::endl;
        forward_message(message);
    }
}

void Pipeline::forward_message(Message message)
{
    sender_.send(message);
}

void Pipeline::enqueue_message(Message message)
{
    queue_.push(std::move(message));
}

} // namespace pipeline
