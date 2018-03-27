#include "ConnectionManager.hpp"

#include <fluid/of10/openflow-10.h>
#include <fluid/of13/openflow-13.h>

#include <cassert>
#include <iostream>

Client::Client(ConnectionManager* connection_manager, uint32_t thread_num,
               OFServerSettings settings):
    OFClient(thread_num, settings), connection_manager_(connection_manager)
{
    std::cout<<"Starting Client"<<std::endl;
    OFClient::start();
}

Client::~Client()
{
    std::cout<<"Finishing Client"<<std::endl;
    OFClient::stop();
}

void Client::connection_callback(OFConnection* connection,
                                 OFConnection::Event type)
{
    connection_manager_->onControllerConnection(connection, type);
}

void Client::message_callback(OFConnection* connection, uint8_t type,
                              void* data, size_t len)
{
    RawMessage message(type, data, len);
    connection_manager_->onControllerMessage(connection, message);
}

Server::Server(ConnectionManager* connection_manager,
               const std::string& address,
               uint32_t port, uint32_t thread_num, OFServerSettings settings):
    OFServer(address.c_str(), port, thread_num, false, settings),
    connection_manager_(connection_manager)
{
    std::cout<<"Starting Server"<<std::endl;
    OFServer::start();
}

Server::~Server()
{
    std::cout<<"Finishing Server"<<std::endl;
    OFServer::stop();
}

void Server::connection_callback(OFConnection* connection,
                                 OFConnection::Event type)
{
    connection_manager_->onSwitchConnection(connection, type);
}

void Server::message_callback(OFConnection* connection, uint8_t type,
                              void* data, size_t len)
{
    RawMessage message(type, data, len);
    connection_manager_->onSwitchMessage(connection, message);
}

ConnectionManager::ConnectionManager(ProxySettings settings,
                                     EventQueue& event_queue):
    settings_(std::move(settings)), event_queue_(event_queue)
{
    auto thread_num = 1;
    // TODO: if proxy will use handshake then we need to handle different controller and switch versions
    auto of_settings = fluid_base::OFServerSettings()
        .supported_version(fluid_msg::of10::OFP_VERSION)
        .supported_version(fluid_msg::of13::OFP_VERSION)
        .liveness_check(false)
        .handshake(false)
        .dispatch_all_messages(true)
        .keep_data_ownership(false);
    auto client_settings = of_settings.is_controller(false);
    auto server_settings = of_settings.is_controller(true);

    client_ = std::make_unique<Client>(
        this, thread_num, client_settings
    );

    server_ = std::make_unique<Server>(
        this, settings_.proxy_address, settings_.proxy_port,
        thread_num, server_settings
    );
}


void ConnectionManager::sendToController(ConnectionId id, RawMessage message)
{
    auto it = connections_.find(id);
    if (connections_.end() != it) {
        std::cout << "Send to controller: connection id="
                  << id << std::endl;
        send(it->second.controller_connection, message);
    }
    else {
        std::cerr << "Send message error: No proxy connection id="
                  << id << std::endl;
    }
}

void ConnectionManager::sendToSwitch(ConnectionId id, RawMessage message)
{
    auto it = connections_.find(id);
    if (connections_.end() != it) {
        std::cout << "Send to switch: connection id="
                  << id << std::endl;
        send(it->second.switch_connection, message);
    }
    else {
        std::cerr << "Send message error: No proxy connection id="
                  << id << std::endl;
    }
}

void ConnectionManager::onControllerConnection(OFConnection* connection,
                                               OFConnection::Event type)
{
    switch (type) {
    case OFConnection::EVENT_STARTED:
        std::cout << "Controller connection id=" << get_id(connection) << " from "
                  << connection->get_peer_address()
                  << " started" << std::endl;
        accept_controller_connection(connection);
        break;
    case OFConnection::EVENT_ESTABLISHED:
        std::cout << "Controller connection id=" << get_id(connection) << " from "
                  << connection->get_peer_address()
                  << " established" << std::endl;
        break;
    case OFConnection::EVENT_FAILED_NEGOTIATION:
        std::cout << "Controller connection id=" << get_id(connection) << " from "
                  << connection->get_peer_address()
                  << ": failed version negotiation" << std::endl;
        break;
    case OFConnection::EVENT_CLOSED:
        delete_proxy_connection(connection);
        std::cout << "Controller connection id=" << get_id(connection) << " from "
                  << connection->get_peer_address()
                  << " closed by the user" << std::endl;
        break;
    case OFConnection::EVENT_DEAD:
        delete_proxy_connection(connection);
        std::cout << "Controller connection id=" << get_id(connection) << " from "
                  << connection->get_peer_address()
                  << " closed due to inactivity" << std::endl;
        break;
    }
}

void ConnectionManager::onSwitchConnection(OFConnection* connection,
                                           OFConnection::Event type)
{
    switch (type) {
    case OFConnection::EVENT_STARTED:
        std::cout << "Switch connection id=" << get_id(connection) << " from "
                  << connection->get_peer_address()
                  << " started" << std::endl;
        start_controller_connection(connection);
        break;
    case OFConnection::EVENT_ESTABLISHED:
        std::cout << "Switch connection id=" << get_id(connection) << " from "
                  << connection->get_peer_address()
                  << " established" << std::endl;
        break;
    case OFConnection::EVENT_FAILED_NEGOTIATION:
        std::cout << "Switch connection id=" << get_id(connection) << " from "
                  << connection->get_peer_address()
                  << ": failed version negotiation" << std::endl;
        break;
    case OFConnection::EVENT_CLOSED:
        std::cout << "Switch connection id=" << get_id(connection) << " from "
                  << connection->get_peer_address()
                  << " closed by the user" << std::endl;
        delete_proxy_connection(connection);
        break;
    case OFConnection::EVENT_DEAD:
        std::cout << "Switch connection id=" << get_id(connection) << " from "
                  << connection->get_peer_address()
                  << " closed due to inactivity" << std::endl;
        delete_proxy_connection(connection);
        break;
    }
}

void ConnectionManager::onControllerMessage(OFConnection* connection,
                                            RawMessage message)
{
    auto connection_id = get_id(connection);
    auto it = connections_.find(connection_id);
    if (connections_.end() != it) {
        std::cout << "Received from controller: connection id="
                  << connection_id << std::endl;

        event_queue_.push(std::make_shared<MessageEvent>(
            connection_id, Origin::FROM_CONTROLLER, std::move(message)
        ));
    }
    else {
        std::cerr << "Controller message proxy error: No proxy connection id="
                  << connection_id << std::endl;
    }
}

void ConnectionManager::onSwitchMessage(OFConnection* connection,
                                        RawMessage message)
{
    auto connection_id = get_id(connection);
    auto waiting_it = waiting_connections_.find(connection_id);
    if (waiting_connections_.end() != waiting_it) {
        std::cout << "Enqueued from switch: connection id="
                  << connection_id << std::endl;

        // Queue messages
        auto& message_queue = waiting_it->second.message_queue;
        message_queue.push(message);
    }
    else {
        auto it = connections_.find(connection_id);
        if (connections_.end() != it) {
            std::cout << "Received from switch: connection id="
                      << connection_id << std::endl;

            event_queue_.push(std::make_shared<MessageEvent>(
                connection_id, Origin::FROM_SWITCH, std::move(message)
            ));
        }
        else {
            std::cerr << "Switch message proxy error: No proxy connection id="
                      << connection_id << std::endl;
        }
    }
}

void ConnectionManager::start_controller_connection(OFConnection* switch_connection)
{
    auto connection_id = get_id(switch_connection);
    waiting_connections_.emplace(
        connection_id, WaitingConnection(switch_connection)
    );

    client_->add_connection(
        connection_id, settings_.controller_address, settings_.controller_port
    );
}

void ConnectionManager::accept_controller_connection(OFConnection* controller_connection)
{
    auto connection_id = get_id(controller_connection);
    auto it = waiting_connections_.find(connection_id);
    if (waiting_connections_.end() != it) {
        auto switch_connection = it->second.connection;
        add_proxy_connection(controller_connection, switch_connection);

        // Send delayed messages
        auto& message_queue = it->second.message_queue;
        while (not message_queue.empty()) {
            std::cout << "Send to switch (QUEUED): connection id="
                      << connection_id << std::endl;

            auto& message = message_queue.front();
            event_queue_.push(std::make_shared<MessageEvent>(
                connection_id, Origin::FROM_SWITCH, std::move(message)
            ));
            message_queue.pop();
        }
        waiting_connections_.erase(it);
    }
    else {
        // Expired connection
        controller_connection->close();
    }
}

void
ConnectionManager::add_proxy_connection(OFConnection* controller_connection,
                                        OFConnection* switch_connection)
{
    assert(get_id(controller_connection) == get_id(switch_connection));
    auto connection_id = get_id(controller_connection);
    auto it = connections_.find(connection_id);
    if (connections_.end() == it) {
        connections_.emplace(
            connection_id,
            Connection{controller_connection, switch_connection}
        );

        event_queue_.push(std::make_shared<ConnectionEvent>(
            connection_id, ConnectionEventType::NEW
        ));

        std::cout << "Proxy connection id=" << get_id(switch_connection)
                  << " started" << std::endl;
    }
    else {
        std::cerr << "Connection id=" << connection_id
                  << " is already presented" << std::endl;
    }
}

void ConnectionManager::delete_proxy_connection(OFConnection* connection)
{
    auto id = get_id(connection);
    auto waiting_it = waiting_connections_.find(id);
    auto connection_it = connections_.find(id);
    // There cannot be same waiting connections and proxy connections
    assert((waiting_connections_.end() != waiting_it and
            connections_.end() == connection_it) or
           (waiting_connections_.end() == waiting_it and
            connections_.end() != connection_it));

    if (waiting_connections_.end() != waiting_it) {
        // TODO: delete old connections and their messages
        waiting_it->second.connection->close();
        waiting_connections_.erase(waiting_it);
    }
    else if (connections_.end() != connection_it) {
        connection_it->second.controller_connection->close();
        connection_it->second.switch_connection->close();
        connections_.erase(connection_it);

        event_queue_.push(std::make_shared<ConnectionEvent>(
            id, ConnectionEventType::CLOSED
        ));

        std::cout << "Proxy connection id=" << id
                  << " closed" << std::endl;
    }
}

void ConnectionManager::send(OFConnection* connection, RawMessage message)
{
    connection->send(message.data(), message.len);
    // TODO: delete message
}
