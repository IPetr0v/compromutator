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
    proxy_settings.controller_address = "127.0.0.1";
    proxy_settings.controller_port = 6655;
    proxy_settings.proxy_address = "127.0.0.1";
    proxy_settings.proxy_port = 6653;

    using H = HeaderSpace;

    cout<<(H("xxxxxxxx") - H("00000000")) <<endl;
    cout<<((H("xxxxxxxx") - H("xxxxx000")) - H("xxxx0000")) +
          ((H("xxxxxxx0") - H("xxxxx000")) - H("xxxx0000")) +
          ((H("xxxxxx00") - H("xxxxx000")) - H("xxxx0000")) <<endl;
    cout<<H("xx0011xx")<<" == "
        <<H("xx001100") + H("xx001101") + H("xx001110") + H("xx001111")<<endl;

    {
        auto hs = hs_create(1u);
        hs_add(hs, array_from_str("xx001100"));
        hs_add(hs, array_from_str("xx001101"));
        hs_add(hs, array_from_str("xx001110"));
        hs_add(hs, array_from_str("xx001111"));
        hs_compact(hs);
        cout << hs_to_str(hs) << endl;
    }

    {
        auto hs = hs_create(1u);
        hs_add(hs, array_from_str("xxxxx000"));
        hs_diff(hs, array_from_str("xxxx0000"));
        cout << hs_to_str(hs) << " -> ";
        hs_compact(hs);
        cout << hs_to_str(hs) << endl;
    }

    {
        auto hs = hs_create(1u);
        hs_add(hs, array_from_str("xxxxxxx0"));
        hs_add(hs, array_from_str("xxxxxxx1"));
        hs_compact(hs);
        cout << hs_to_str(hs) << endl;
    }

    {
        auto hs = hs_create(1u);
        hs_add(hs, array_from_str("xxxxxxxx"));
        hs_add(hs, array_from_str("xxxxxxx1"));
        hs_compact(hs);
        cout << hs_to_str(hs) << endl;
    }
    cout << "=======" << endl;
    {
        array_t *extra;
        auto array1 = array_from_str("xxxxxxx0");
        auto array2 = array_from_str("xxxxxxx1");
        array_combine(&array1, &array2, &extra, nullptr, 1u);
        if (array1)
            cout << array_to_str(array1, 1u, false) << " | ";
        if (array2)
            cout << array_to_str(array2, 1u, false) << " | ";
        if (extra)
            cout << array_to_str(extra, 1u, false);
        cout << endl;
    }

    {
        array_t *extra;
        auto array1 = array_from_str("xxxxxxxx");
        auto array2 = array_from_str("xxxxxxx1");
        array_combine(&array1, &array2, &extra, nullptr, 1u);
        if (array1)
            cout << array_to_str(array1, 1u, false) << " | ";
        if (array2)
            cout << array_to_str(array2, 1u, false) << " | ";
        if (extra)
            cout << array_to_str(extra, 1u, false);
        cout << endl;
    }

    /*auto arr = array_from_str("xxxxxxx0");
    auto from = array_from_str("1xxxxxxx");
    array_one_bit_subtract(arr, from, 1u);
    cout<<array_to_str(from, 1u, false)<<endl;*/

    // TODO: fix unhandled exception on destructor!
    //Compromutator compromutator(proxy_settings);
    //compromutator.run();

    return 0;
}
