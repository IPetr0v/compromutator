#include "Proxy.hpp"

#include <cassert>
#include <string>
#include <sstream>

ProxyClient::ProxyClient(Proxy* proxy, uint32_t thread_num,
                         OFServerSettings settings):
    OFClient(thread_num, settings), proxy_(proxy)
{
    std::cout<<"Starting ProxyClient"<<std::endl;
    OFClient::start();
}

ProxyClient::~ProxyClient()
{
    std::cout<<"Finishing ProxyClient"<<std::endl;
    OFClient::stop();
}

void ProxyClient::connection_callback(OFConnection* connection,
                                      OFConnection::Event type)
{
    proxy_->addJob(std::make_unique<ProxyConnectionJob>(
        Origin::FROM_CONTROLLER, connection, type
    ));
}

void ProxyClient::message_callback(OFConnection* connection, uint8_t type,
                                   void* data, size_t len)
{
    Message message(type, data, len);
    proxy_->addJob(std::make_unique<ProxyMessageJob>(
        Origin::FROM_CONTROLLER, connection, message
    ));
}

ProxyServer::ProxyServer(Proxy* proxy, const std::string& address,
                         uint32_t port, uint32_t thread_num,
                         OFServerSettings settings):
    OFServer(address.c_str(), port, thread_num, false, settings), proxy_(proxy)
{
    std::cout<<"Starting ProxyServer"<<std::endl;
    OFServer::start();
}

ProxyServer::~ProxyServer()
{
    std::cout<<"Finishing ProxyServer"<<std::endl;
    OFServer::stop();
}

void ProxyServer::connection_callback(OFConnection* connection,
                                      OFConnection::Event type)
{
    proxy_->addJob(std::make_unique<ProxyConnectionJob>(
        Origin::FROM_SWITCH, connection, type
    ));
}

void ProxyServer::message_callback(OFConnection* connection, uint8_t type,
                                   void* data, size_t len)
{
    Message message(type, data, len);
    proxy_->addJob(std::make_unique<ProxyMessageJob>(
        Origin::FROM_SWITCH, connection, message
    ));
}

void ConcurrentJobQueue::addJob(std::unique_ptr<ProxyJob>&& job)
{
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(std::move(job));
    is_available_.notify_one();
}

void ConcurrentJobQueue::wait()
{
    std::unique_lock<std::mutex> wait_lock(mutex_);
    is_available_.wait(wait_lock);
}

void ConcurrentJobQueue::runJobs(Proxy* proxy)
{
    std::unique_ptr<ProxyJob> current_job;
    while (not queue_.empty()) {
        {
            std::lock_guard<std::mutex> job_lock(mutex_);
            current_job = std::move(queue_.front());
            queue_.pop();
        }

        current_job->run(proxy);
    }
}

Proxy::Proxy()
{
    controller_address_ = "127.0.0.1";
    controller_port_ = 6655;
    proxy_address_ = "127.0.0.1";
    proxy_port_ = 6653;

    auto thread_num = 1;
    // TODO: if proxy will use handshake then we need to handle different controller and switch versions
    auto settings = fluid_base::OFServerSettings()
        .supported_version(fluid_msg::of10::OFP_VERSION)
        .supported_version(fluid_msg::of13::OFP_VERSION)
        .liveness_check(false)
        .handshake(false)
        .dispatch_all_messages(true)
        .keep_data_ownership(false);
    auto client_settings = settings.is_controller(false);
    auto server_settings = settings.is_controller(true);

    client_ = std::make_unique<ProxyClient>(
        this, thread_num, client_settings
    );

    server_ = std::make_unique<ProxyServer>(
        this, proxy_address_, proxy_port_, thread_num, server_settings
    );
}

void Proxy::start()
{
    is_running_ = true;
    while (is_running_) {
        job_queue_.wait();
        job_queue_.runJobs(this);
    }
}

void Proxy::stop()
{
    client_->stop();
    server_->stop();
    is_running_ = false;

    // TODO: delete jobs from queue
}

void Proxy::addJob(std::unique_ptr<ProxyJob>&& job)
{
    job_queue_.addJob(std::move(job));
}

void Proxy::onControllerConnection(OFConnection* controller_connection,
                                   OFConnection::Event type)
{
    switch (type) {
    case OFConnection::EVENT_STARTED:
        std::cout << "Controller connection id=" << controller_connection->get_id() << " from "
                  << controller_connection->get_peer_address()
                  << " started" << std::endl;
        accept_controller_connection(controller_connection);
        break;
    case OFConnection::EVENT_ESTABLISHED:
        std::cout << "Controller connection id=" << controller_connection->get_id() << " from "
                  << controller_connection->get_peer_address()
                  << " established" << std::endl;
        break;
    case OFConnection::EVENT_FAILED_NEGOTIATION:
        std::cout << "Controller connection id=" << controller_connection->get_id() << " from "
                  << controller_connection->get_peer_address()
                  << ": failed version negotiation" << std::endl;
        break;
    case OFConnection::EVENT_CLOSED:
        delete_proxy_connection(controller_connection->get_id());
        std::cout << "Controller connection id=" << controller_connection->get_id() << " from "
                  << controller_connection->get_peer_address()
                  << " closed by the user" << std::endl;
        break;
    case OFConnection::EVENT_DEAD:
        delete_proxy_connection(controller_connection->get_id());
        std::cout << "Controller connection id=" << controller_connection->get_id() << " from "
                  << controller_connection->get_peer_address()
                  << " closed due to inactivity" << std::endl;
        break;
    }
}

void Proxy::onSwitchConnection(OFConnection* switch_connection,
                               OFConnection::Event type)
{
    switch (type) {
    case OFConnection::EVENT_STARTED:
        std::cout << "Switch connection id=" << switch_connection->get_id() << " from "
                  << switch_connection->get_peer_address()
                  << " started" << std::endl;
        start_controller_connection(switch_connection);
        break;
    case OFConnection::EVENT_ESTABLISHED:
        std::cout << "Switch connection id=" << switch_connection->get_id() << " from "
                  << switch_connection->get_peer_address()
                  << " established" << std::endl;
        break;
    case OFConnection::EVENT_FAILED_NEGOTIATION:
        std::cout << "Switch connection id=" << switch_connection->get_id() << " from "
                  << switch_connection->get_peer_address()
                  << ": failed version negotiation" << std::endl;
        break;
    case OFConnection::EVENT_CLOSED:
        std::cout << "Switch connection id=" << switch_connection->get_id() << " from "
                  << switch_connection->get_peer_address()
                  << " closed by the user" << std::endl;
        delete_proxy_connection(switch_connection->get_id());
        break;
    case OFConnection::EVENT_DEAD:
        std::cout << "Switch connection id=" << switch_connection->get_id() << " from "
                  << switch_connection->get_peer_address()
                  << " closed due to inactivity" << std::endl;
        delete_proxy_connection(switch_connection->get_id());
        break;
    }
}

void Proxy::onControllerMessage(OFConnection* controller_connection,
                                Message message)
{
    if (message.type == of13::OFPT_HELLO) {
        //// Do not proxy handshake
        //return;
        std::cout << "Proxying HELLO from the controller" << std::endl;
    }
    if (message.type == of13::OFPT_FEATURES_REQUEST) {
        std::cout << "Proxying FEATURES_REQUEST from the controller" << std::endl;
    }

    auto connection_id = controller_connection->get_id();
    auto it = connections_.find(connection_id);
    if (connections_.end() != it) {
        auto switch_connection = it->second.switch_connection;
        send(switch_connection, message);
    }
    else {
        std::cerr << "Controller message proxy error: No proxy connection id="
                  << connection_id << std::endl;
    }
}

void Proxy::onSwitchMessage(OFConnection* switch_connection, Message message)
{
    if (message.type == of13::OFPT_HELLO) {
        //// Do not proxy handshake
        //return;
        std::cout << "Proxying HELLO from the switch" << std::endl;
    }
    if (message.type == of13::OFPT_FEATURES_REPLY) {
        std::cout << "Proxying FEATURES_REPLY from the switch" << std::endl;
    }

    auto connection_id = switch_connection->get_id();
    auto waiting_it = waiting_connections_.find(connection_id);
    if (waiting_connections_.end() != waiting_it) {
        // Queue messages
        auto& message_queue = waiting_it->second.message_queue;
        message_queue.push(message);
    }
    else {
        auto it = connections_.find(connection_id);
        if (connections_.end() != it) {
            auto controller_connection = it->second.controller_connection;
            send(controller_connection, message);

        }
        else {
            std::cerr << "Switch message proxy error: No proxy connection id="
                      << connection_id << std::endl;
        }
    }
}

void Proxy::start_controller_connection(OFConnection* switch_connection)
{
    auto connection_id = switch_connection->get_id();
    waiting_connections_.emplace(
        connection_id, WaitingConnection(switch_connection)
    );

    // TODO: change this temporary solution (create thread in fluid)
    std::thread connection_thread([this, connection_id, switch_connection]() {
        client_->add_connection(connection_id,
                                controller_address_,
                                controller_port_);
    });

    // Init thread name
    std::ostringstream thread_name;
    thread_name << "Proxy Connection";
    pthread_setname_np(connection_thread.native_handle(),
                       thread_name.str().c_str());

    connection_thread.detach();
}

void Proxy::accept_controller_connection(OFConnection* controller_connection)
{
    auto connection_id = controller_connection->get_id();
    auto it = waiting_connections_.find(connection_id);
    if (waiting_connections_.end() != it) {
        auto switch_connection = it->second.connection;

        // Send delayed messages
        auto& message_queue = it->second.message_queue;
        while (not message_queue.empty()) {
            auto& message = message_queue.front();
            send(controller_connection, message);
            message_queue.pop();
        }

        waiting_connections_.erase(it);
        add_proxy_connection(controller_connection, switch_connection);
    }
    else {
        // Expired connection
        controller_connection->close();
    }
}

void Proxy::add_proxy_connection(OFConnection* controller_connection,
                                 OFConnection* switch_connection)
{
    assert(controller_connection->get_id() == switch_connection->get_id());
    auto connection_id = controller_connection->get_id();
    auto it = connections_.find(connection_id);
    if (connections_.end() == it) {
        connections_.emplace(
            connection_id,
            ProxyConnection{controller_connection, switch_connection}
        );

        std::cout << "Proxy connection id=" << switch_connection->get_id()
                  << " started" << std::endl;
    }
    else {
        std::cerr << "Connection id=" << connection_id
                  << " is already presented" << std::endl;
    }
}

void Proxy::delete_proxy_connection(ConnectionId id)
{
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

        std::cout << "Proxy connection id=" << id
                  << " closed" << std::endl;
    }
}

void Proxy::send(OFConnection* connection, Message message)
{
    connection->send(message.data.get(), message.len);
}
