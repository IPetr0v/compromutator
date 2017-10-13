
#include <typeinfo>

#include <string.h>
#include <iostream>
#include <vector>
#include <map>

#include "Common.hpp"
#include "./detector/Detector.hpp"

int main(int argc, char* argv[])
{
    int length = 1;
    
    Detector detector(length);
    detector.addSwitch(1, {1,2,3,4});
    detector.addSwitch(2, {1,2,3,4});
    detector.addSwitch(3, {1,2,3,4});
    detector.addSwitch(4, {1,2,3,4});
    
    detector.addLink(1,2, 2,2);
    detector.addLink(1,3, 4,3);
    detector.addLink(1,4, 3,4);
    detector.addLink(3,3, 2,3);
    detector.addLink(3,2, 4,2);
    detector.addLink(4,4, 2,4);
    
    TableId front_table = 0;
    uint16_t table_miss_priority = 0;
    NetworkSpace table_miss_match(HeaderSpace("xxxxxxxx"));
    std::vector<Action> table_miss_actions;
    
    for (auto& switch_id : {1,2,3,4}) {
        detector.addRule(switch_id, front_table,
                         table_miss_priority,
                         table_miss_match,
                         table_miss_actions);
    }
}
