#pragma once

#include "Event.hpp"

#include <fluid/OFServer.hh>
#include <fluid/OFClient.hh>
#include <fluid/OFConnection.hh>

#include <memory>
#include <unordered_map>

using namespace fluid_base;

class ConnectionManager;

class Client : public OFClient
{
public:
    Client(ConnectionManager* connection_manager, uint32_t thread_num,
           OFServerSettings settings);
    ~Client();

private:
    ConnectionManager* connection_manager_;

    void connection_callback(OFConnection* connection,
                             OFConnection::Event type) override;
    void message_callback(OFConnection* connection, uint8_t type,
                          void* data, size_t len) override;

};

class Server : public OFServer
{
public:
    Server(ConnectionManager* connection_manager, const std::string& address,
           uint32_t port, uint32_t thread_num, OFServerSettings settings);
    ~Server();

private:
    ConnectionManager* connection_manager_;

    void connection_callback(OFConnection* connection,
                             OFConnection::Event type) override;
    void message_callback(OFConnection* connection, uint8_t type,
                          void* data, size_t len) override;

};

struct ProxySettings
{
    std::string controller_address;
    uint32_t controller_port;
    std::string proxy_address;
    uint32_t proxy_port;
};

class ConnectionManager
{
private:
    struct WaitingConnection
    {
        explicit WaitingConnection(OFConnection* connection):
            connection(connection) {}
        OFConnection* connection;
        std::queue<RawMessage> message_queue;
    };

    struct Connection
    {
        OFConnection* controller_connection;
        OFConnection* switch_connection;
    };

public:
    ConnectionManager(ProxySettings settings, EventQueue& event_queue);

    void sendToController(ConnectionId id, RawMessage message);
    void sendToSwitch(ConnectionId id, RawMessage message);

    void onControllerConnection(OFConnection* connection,
                                OFConnection::Event type);
    void onSwitchConnection(OFConnection* connection,
                            OFConnection::Event type);

    void onControllerMessage(OFConnection* connection,
                             RawMessage message);
    void onSwitchMessage(OFConnection* connection,
                         RawMessage message);

private:
    ProxySettings settings_;
    std::unique_ptr<Client> client_;
    std::unique_ptr<Server> server_;

    std::unordered_map<ConnectionId, Connection> connections_;
    std::unordered_map<ConnectionId, WaitingConnection> waiting_connections_;

    EventQueue& event_queue_;

    ConnectionId get_id(OFConnection* connection) {return connection->get_id();}
    void start_controller_connection(OFConnection* switch_connection);
    void accept_controller_connection(OFConnection* controller_connection);

    // TODO: make connections with different types to avoid mistakes
    void add_proxy_connection(OFConnection* controller_connection,
                              OFConnection* switch_connection);
    void delete_proxy_connection(OFConnection* connection);

    void send(OFConnection* connection, RawMessage message);

};
