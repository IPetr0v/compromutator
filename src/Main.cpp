#include "Compromutator.hpp"

#include <array>
#include <typeinfo>
#include <iostream>
#include <list>
#include <map>
#include <cstring>
#include <vector>
#include <unistd.h>

using namespace std;

int main(int argc, char* argv[])
{
    HeaderSpace::GLOBAL_LENGTH = Mapping::HEADER_SIZE;

    ProxySettings proxy_settings;
    proxy_settings.controller_address = "127.0.0.1";//"192.168.131.216";
    proxy_settings.controller_port = 6666;//22822;
    proxy_settings.proxy_address = "127.0.0.1";
    proxy_settings.proxy_port = 6653;

    // TODO: fix unhandled exception on destructor!
    Compromutator compromutator(proxy_settings);
    compromutator.run();

    return 0;
}
