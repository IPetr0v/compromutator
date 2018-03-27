#include "Compromutator.hpp"

#include <typeinfo>
#include <iostream>
#include <list>
#include <map>
#include <string.h>
#include <vector>
#include <unistd.h>

using namespace std;

int main(int argc, char* argv[])
{
    ProxySettings proxy_settings;
    proxy_settings.controller_address = "127.0.0.1";
    proxy_settings.controller_port = 6655;
    proxy_settings.proxy_address = "127.0.0.1";
    proxy_settings.proxy_port = 6653;

    Compromutator compromutator(proxy_settings);

    return 0;
}
