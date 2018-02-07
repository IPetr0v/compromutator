#pragma once

#include <fluid/OFServer.hh>
#include <fluid/OFClient.hh>
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

using ConnectionId = uint32_t;
using OFConnectionPtr = std::shared_ptr<OFConnection>;
/*using MessageCallback = std::function<void(OFConnection*,
                                           OFConnection::Event)>;*/

class Proxy;
class ProxyJob;

class ProxyClient : public OFClient
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

class ProxyServer : public OFServer
{
public:
    ProxyServer(Proxy* proxy, const std::string& address, uint32_t port,
                uint32_t thread_num, OFServerSettings settings);
    ~ProxyServer();

private:
    Proxy* proxy_;
    ProxyClient* proxy_client_;

    void connection_callback(OFConnection* connection,
                             OFConnection::Event type) override;
    void message_callback(OFConnection* connection, uint8_t type,
                          void* data, size_t len) override;

};

struct ProxyConnection
{
    OFConnection* controller_connection;
    OFConnection* switch_connection;
};

struct Message
{
    Message(uint8_t type, void* data, size_t len):
        type(type), data(data), len(len) {}

    uint8_t type;
    void* data;
    size_t len;
};

// class ProxyController
class Proxy
{
public:
    Proxy();

    void start();
    void stop();

    void addJob(std::unique_ptr<ProxyJob>&& job);

    void onControllerConnection(OFConnection* connection,
                                OFConnection::Event type);
    void onSwitchConnection(OFConnection* connection,
                            OFConnection::Event type);

    void onControllerMessage(OFConnection* connection, Message message);
    void onSwitchMessage(OFConnection* connection, Message message);

private:
    std::string controller_address_;
    uint32_t controller_port_;
    std::string proxy_address_;
    uint32_t proxy_port_;

    std::unique_ptr<ProxyClient> client_;
    std::unique_ptr<ProxyServer> server_;

    OFConnection* controller_connection_;
    OFConnection* switch_connection_;

    std::atomic_bool is_running_;
    std::mutex job_mutex_;
    std::queue<std::unique_ptr<ProxyJob>> job_queue_;
    std::condition_variable job_available_;

    void connect_to_controller(OFConnection* connection);

};
enum class Origin
{
    FROM_CONTROLLER,
    FROM_SWITCH
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
        ProxyJob(origin, connection), event_type_(event_type) {
        if (origin == Origin::FROM_CONTROLLER) {
            std::cout << "New Controller ProxyConnectionJob" << std::endl;
        }
        else {
            std::cout << "New Switch ProxyConnectionJob" << std::endl;
        }
    }

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
    ProxyMessageJob(Origin origin, OFConnection* connection,
                    uint8_t message_type, void* message, size_t message_len):
        ProxyJob(origin, connection),
        message_(message_type, message, message_len) {
        if (origin == Origin::FROM_CONTROLLER) {
            std::cout << "New Controller ProxyMessageJob" << std::endl;
        }
        else {
            std::cout << "New Switch ProxyMessageJob" << std::endl;
        }
    }

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
