#include "openflow/Types.hpp"
#include "proxy/Proxy.hpp"
#include "NetworkSpace.hpp"
#include "Detector.hpp"

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
    auto proxy = std::make_unique<Proxy>();
    proxy->start();

    while (true) {
        usleep(1000000);
    }
    return 0;
}
