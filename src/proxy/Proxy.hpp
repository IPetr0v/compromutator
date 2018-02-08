#pragma once

#include <fluid/OFServer.hh>
#include <fluid/OFClient.hh>
#include <fluid/base/BaseOFServer.hh>
#include <fluid/base/BaseOFClient.hh>
#include <fluid/OFConnection.hh>
#include <fluid/of10/openflow-10.h>
#include <fluid/of13/openflow-13.h>

#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <queue>

#include <iostream>
using namespace fluid_base;
using namespace fluid_msg;

using ConnectionId = uint32_t;

class Proxy;
class ProxyJob;

class ProxyClient : public OFClient//, public OFHandler
{
public:
    ProxyClient(Proxy* proxy, uint32_t thread_num, OFServerSettings settings);
    ~ProxyClient();

private:
    Proxy* proxy_;

    void connection_callback(OFConnection* connection,
                             OFConnection::Event type) override;
    void message_callback(OFConnection* connection, uint8_t type,
                          void* data, size_t len) override;

};

class ProxyServer : public OFServer//, public OFHandler
{
public:
    ProxyServer(Proxy* proxy, const std::string& address, uint32_t port,
                uint32_t thread_num, OFServerSettings settings);
    ~ProxyServer();

private:
    Proxy* proxy_;

    void connection_callback(OFConnection* connection,
                             OFConnection::Event type) override;
    void message_callback(OFConnection* connection, uint8_t type,
                          void* data, size_t len) override;

};

struct Message
{
    Message(uint8_t type, void* data, size_t len):
        type(type), data(reinterpret_cast<uint8_t*>(data)), len(len) {}

    uint8_t type;
    std::shared_ptr<uint8_t> data;
    size_t len;
};

struct ProxyConnection
{
    OFConnection* controller_connection;
    OFConnection* switch_connection;
};

enum class ProxyCommand
{
    FORWARD,
    DROP
};

struct WaitingConnection
{
    explicit WaitingConnection(OFConnection* connection):
        connection(connection) {}
    OFConnection* connection;
    std::queue<Message> message_queue;
};

class ConcurrentJobQueue
{
public:
    void addJob(std::unique_ptr<ProxyJob>&& job);
    void wait();
    void runJobs(Proxy* proxy);

private:
    std::mutex mutex_;
    std::condition_variable is_available_;
    std::queue<std::unique_ptr<ProxyJob>> queue_;
};

class Proxy
{
public:
    Proxy();

    void start();
    void stop();

    void addJob(std::unique_ptr<ProxyJob>&& job);

    void sendToController(ConnectionId id, Message message) {}
    void sendToSwitch(ConnectionId id, Message message) {}

    void onControllerConnection(OFConnection* controller_connection,
                                OFConnection::Event type);
    void onSwitchConnection(OFConnection* switch_connection,
                            OFConnection::Event type);

    void onControllerMessage(OFConnection* controller_connection,
                             Message message);
    void onSwitchMessage(OFConnection* switch_connection,
                         Message message);

private:
    std::string controller_address_;
    uint32_t controller_port_;
    std::string proxy_address_;
    uint32_t proxy_port_;

    std::atomic_bool is_running_;
    ConcurrentJobQueue job_queue_;

    std::unique_ptr<ProxyClient> client_;
    std::unique_ptr<ProxyServer> server_;

    std::map<ConnectionId, ProxyConnection> connections_;
    std::map<ConnectionId, WaitingConnection> waiting_connections_;

    void start_controller_connection(OFConnection* switch_connection);
    void accept_controller_connection(OFConnection* controller_connection);

    // TODO: make conections with different types to avoid mistakes
    void add_proxy_connection(OFConnection* controller_connection,
                              OFConnection* switch_connection);
    void delete_proxy_connection(ConnectionId id);

    void send(OFConnection* connection, Message message);

};

enum class Origin
{
    FROM_CONTROLLER,
    FROM_SWITCH
};

enum class Destination
{
    TO_CONTROLLER,
    TO_SWITCH
};

class ProxyJob
{
public:
    ProxyJob(Origin origin, OFConnection* connection):
        origin_(origin), connection_(connection) {}

    virtual void run(Proxy* proxy) = 0;

protected:
    Origin origin_;
    OFConnection* connection_;
};

class ProxyConnectionJob : public ProxyJob
{
public:
    ProxyConnectionJob(Origin origin, OFConnection* connection,
                       OFConnection::Event event_type):
        ProxyJob(origin, connection), event_type_(event_type) {}

    void run(Proxy* proxy) override {
        if (Origin::FROM_CONTROLLER == origin_) {
            proxy->onControllerConnection(connection_, event_type_);
        }
        else {
            proxy->onSwitchConnection(connection_, event_type_);
        }
    }

private:
    OFConnection::Event event_type_;
};

class ProxyMessageJob : public ProxyJob
{
public:
    ProxyMessageJob(Origin origin, OFConnection* connection, Message message):
        ProxyJob(origin, connection), message_(std::move(message)) {}

    void run(Proxy* proxy) override {
        if (Origin::FROM_CONTROLLER == origin_) {
            proxy->onControllerMessage(connection_, message_);
        }
        else {
            proxy->onSwitchMessage(connection_, message_);
        }
    }

private:
    Message message_;
};

// TODO: add ProxyScheduler class for everyone to include and use
// TODO: implement OutgoingMessageJob, and change names to ConnectionJob and IncomingMessageJob
