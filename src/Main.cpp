#include "Compromutator.hpp"

#include <array>
#include <typeinfo>
#include <iostream>
#include <list>
#include <map>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <cxxopts.hpp>

using namespace std;

int main(int argc, char* argv[])
{
    HeaderSpace::GLOBAL_LENGTH = Mapping::HEADER_SIZE;

    cxxopts::Options options(
        "compromutator",
        "Tool for creating reliable fake network statistics in SDN");
    options.add_options()
        ("c,controller", "Controller IP",
         cxxopts::value<std::string>()->default_value("127.0.0.1"), "IP")
        ("p,port", "Controller port",
         cxxopts::value<uint32_t>()->default_value("6633"), "PORT")
        ("t,timeout", "Timeout duration in milliseconds",
         cxxopts::value<uint32_t>()->default_value("10"), "MS")
        ("m,measurement_filename", "CSV file to save performance measurements",
         cxxopts::value<std::string>()->default_value(""), "FILE");
    auto result = options.parse(argc, argv);

    ProxySettings proxy_settings;
    proxy_settings.controller_address = "127.0.0.1";//result["controller"].as<std::string>();
    proxy_settings.controller_port = 6633;//result["port"].as<uint32_t>();
    proxy_settings.proxy_address = "127.0.0.1";//"0.0.0.0";
    proxy_settings.proxy_port = 6653;

    Compromutator compromutator(
        proxy_settings,
        result["timeout"].as<uint32_t>(),
        result["measurement_filename"].as<std::string>()
     );
    compromutator.run();

    // TODO: fix unhandled exception on destructor!
    return 0;
}
