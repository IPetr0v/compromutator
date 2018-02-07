#include "Proxy.hpp"

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
    proxy_->addJob(std::make_unique<ProxyMessageJob>(
        Origin::FROM_CONTROLLER, connection, type, data, len
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
    //proxy_->onSwitchConnection(connection, type);
    proxy_->addJob(std::make_unique<ProxyConnectionJob>(
        Origin::FROM_SWITCH, connection, type
    ));
}

void ProxyServer::message_callback(OFConnection* connection, uint8_t type,
                                   void* data, size_t len)
{
    //proxy_->onSwitchMessage(conn, type, data, len);
    proxy_->addJob(std::make_unique<ProxyMessageJob>(
        Origin::FROM_SWITCH, connection, type, data, len
    ));
}

Proxy::Proxy()
{
    controller_address_ = "127.0.0.1";
    controller_port_ = 6653;
    proxy_address_ = "127.0.0.1";
    proxy_port_ = 6655;

    auto thread_num = 1;
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
        std::cout << "- Waiting for jobs" << std::endl;
        {
            std::unique_lock<std::mutex> wait_lock(job_mutex_);
            job_available_.wait(wait_lock);
        }

        // Run jobs
        std::cout << "- Running jobs" << std::endl;
        std::unique_ptr<ProxyJob> current_job;
        while (not job_queue_.empty()) {
            {
                std::lock_guard<std::mutex> job_lock(job_mutex_);
                current_job = std::move(job_queue_.front());
                job_queue_.pop();
            }

            current_job->run(this);
        }
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
    std::lock_guard<std::mutex> lock(job_mutex_);
    job_queue_.push(std::move(job));
    job_available_.notify_one();
}

void Proxy::onControllerConnection(OFConnection* connection,
                                   OFConnection::Event type)
{
    controller_connection_ = connection;

    switch (type) {
    case OFConnection::EVENT_STARTED:
        std::cout << "Controller connection id=" << connection->get_id() << " from "
                  << connection->get_peer_address()
                  << " started" << std::endl;
        break;
    case OFConnection::EVENT_ESTABLISHED:
        std::cout << "Controller connection id=" << connection->get_id() << " from "
                  << connection->get_peer_address()
                  << " established" << std::endl;
        break;
    case OFConnection::EVENT_FAILED_NEGOTIATION:
        std::cout << "Controller connection id=" << connection->get_id() << " from "
                  << connection->get_peer_address()
                  << ": failed version negotiation" << std::endl;
        break;
    case OFConnection::EVENT_CLOSED:
        std::cout << "Controller connection id=" << connection->get_id() << " from "
                  << connection->get_peer_address()
                  << " closed by the user" << std::endl;
        break;
    case OFConnection::EVENT_DEAD:
        std::cout << "Controller connection id=" << connection->get_id() << " from "
                  << connection->get_peer_address()
                  << " closed due to inactivity" << std::endl;
        break;
    }
}

void Proxy::onSwitchConnection(OFConnection* connection,
                               OFConnection::Event type)
{
    switch_connection_ = connection;

    switch (type) {
    case OFConnection::EVENT_STARTED:
        connect_to_controller(connection);
        std::cout << "Switch connection id=" << connection->get_id() << " from "
                  << connection->get_peer_address()
                  << " started" << std::endl;
        break;
    case OFConnection::EVENT_ESTABLISHED:
        std::cout << "Switch connection id=" << connection->get_id() << " from "
                  << connection->get_peer_address()
                  << " established" << std::endl;
        break;
    case OFConnection::EVENT_FAILED_NEGOTIATION:
        std::cout << "Switch connection id=" << connection->get_id() << " from "
                  << connection->get_peer_address()
                  << ": failed version negotiation" << std::endl;
        break;
    case OFConnection::EVENT_CLOSED:
        std::cout << "Switch connection id=" << connection->get_id() << " from "
                  << connection->get_peer_address()
                  << " closed by the user" << std::endl;
        break;
    case OFConnection::EVENT_DEAD:
        std::cout << "Switch connection id=" << connection->get_id() << " from "
                  << connection->get_peer_address()
                  << " closed due to inactivity" << std::endl;
        break;
    }
}

void Proxy::connect_to_controller(OFConnection* connection)
{
    // TODO: change this temporary solution
    std::thread connection_thread([this, connection]() {
        client_->add_connection(connection->get_id(),
                                controller_address_, controller_port_);
    });

    // Init thread name
    std::ostringstream thread_name;
    thread_name << "Proxy Connection";
    pthread_setname_np(connection_thread.native_handle(),
                       thread_name.str().c_str());

    connection_thread.detach();
}

void Proxy::onControllerMessage(OFConnection* connection, Message message)
{
    std::cout << "Proxying Message from the controller" << std::endl;
    switch_connection_->send(message.data, message.len);
}

void Proxy::onSwitchMessage(OFConnection* connection, Message message)
{
    // TODO: Check if controller connection is not nullptr
    std::cout << "Proxying Message from the switch" << std::endl;
    controller_connection_->send(message.data, message.len);
}
