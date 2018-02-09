#include "openflow/Types.hpp"
#include "proxy/Proxy.hpp"
#include "proxy/Event.hpp"
#include "NetworkSpace.hpp"
#include "Detector.hpp"

#include <fluid/of10/openflow-10.h>
#include <fluid/of13/openflow-13.h>

#include <chrono>
#include <typeinfo>
#include <iostream>
#include <list>
#include <map>
#include <string.h>
#include <vector>
#include <unistd.h>

using namespace std;

class Compromutator
{
public:
    Compromutator(ProxySettings settings):
        proxy(settings)
    {

    }

    void run() {
        while (true) {
            auto event = proxy.getEvent(100ms);
            switch (event->type) {
            case EventType::TIMEOUT:
                break;
            case EventType::CONNECTION:
                break;
            case EventType::MESSAGE:
                auto message_event = MessageEvent::pointerCast(event);
                if (message_event->origin == Origin::FROM_CONTROLLER) {
                    on_controller_message(
                        message_event->connection_id, message_event->message
                    );
                }
                else {
                    on_switch_message(
                        message_event->connection_id, message_event->message
                    );
                }
                break;
            }
        }
    }

private:
    Proxy proxy;

    void on_controller_message(ConnectionId connection_id, Message message) {
        if (message.type == fluid_msg::of13::OFPT_HELLO) {
            std::cout << "Proxying HELLO from the controller" << std::endl;
        }
        if (message.type == fluid_msg::of13::OFPT_FEATURES_REQUEST) {
            std::cout << "Proxying FEATURES_REQUEST from the controller" << std::endl;
        }

        proxy.sendToSwitch(connection_id, message);
    }

    void on_switch_message(ConnectionId connection_id, Message message) {
        if (message.type == fluid_msg::of13::OFPT_HELLO) {
            std::cout << "Proxying HELLO from the switch" << std::endl;
        }
        if (message.type == fluid_msg::of13::OFPT_FEATURES_REPLY) {
            std::cout << "Proxying FEATURES_REPLY from the switch" << std::endl;
        }

        proxy.sendToController(connection_id, message);
    }

};

int main(int argc, char* argv[])
{
    ProxySettings proxy_settings;
    proxy_settings.controller_address = "127.0.0.1";
    proxy_settings.controller_port = 6655;//6650;
    proxy_settings.proxy_address = "127.0.0.1";
    proxy_settings.proxy_port = 6653;//6651;

    Compromutator compromutator(proxy_settings);

    compromutator.run();

    return 0;
}
