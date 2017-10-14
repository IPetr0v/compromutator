
#include <typeinfo>

#include <iostream>
#include <map>
#include <string.h>
#include <vector>

#include "Common.hpp"
#include "NetworkSpace.hpp"
#include "./detector/Detector.hpp"

int main(int argc, char* argv[])
{
    using namespace std;

    Detector detector;
    cout<<"Construct dependency graph"<<endl;
    using ActionList = std::vector<Action>;

    // Create switches
    detector.addSwitch(1, {1,2,3,4});
    detector.addSwitch(2, {1,2,3,4});
    detector.addSwitch(3, {1,2,3,4});
    detector.addSwitch(4, {1,2,3,4});

    // Add links
    detector.addLink(1,2, 2,2);
    detector.addLink(1,3, 4,3);
    detector.addLink(1,4, 3,4);
    detector.addLink(3,3, 2,3);
    detector.addLink(3,2, 4,2);
    detector.addLink(4,4, 2,4);

    // Define table miss rule
    TableId front_table = 0;
    Priority table_miss_priority = 0;
    NetworkSpace table_miss_domain;
    ActionList table_miss_actions;
    table_miss_actions.emplace_back(Action());
    table_miss_actions[0].type = ActionType::DROP;

    // Add table miss rules
    for (auto& switch_id : {1,2,3,4}) {
        detector.addRule(switch_id, front_table,
                         table_miss_priority,
                         table_miss_domain,
                         table_miss_actions);
    }

    // Forward rule base
    Priority forward_priority = 1;
    NetworkSpace forward_domain;
    ActionList forward_actions;
    forward_actions.emplace_back(Action());
    forward_actions[0].type = ActionType::PORT;

    // 1 -> 2
    forward_domain = NetworkSpace(HeaderSpace("1111xxxx"));
    forward_actions[0].port_id = 2;
    detector.addRule(1, front_table,
                     forward_priority,
                     forward_domain,
                     forward_actions);

    // 1 -> 3
    forward_domain = NetworkSpace(HeaderSpace("0000xxxx"));
    forward_actions[0].port_id = 2;
    detector.addRule(1, front_table,
                     forward_priority,
                     forward_domain,
                     forward_actions);

    // 1 -> 4
    forward_domain = NetworkSpace(HeaderSpace("0011xxxx"));
    forward_actions[0].port_id = 2;
    detector.addRule(1, front_table,
                     forward_priority,
                     forward_domain,
                     forward_actions);

    // 2 -> 3
    forward_domain = NetworkSpace(HeaderSpace("001111xx"));
    forward_actions[0].port_id = 2;
    detector.addRule(2, front_table,
                     forward_priority,
                     forward_domain,
                     forward_actions);

    // 2 -> 4
    forward_domain = NetworkSpace(HeaderSpace("001100xx"));
    forward_actions[0].port_id = 2;
    detector.addRule(2, front_table,
                     forward_priority,
                     forward_domain,
                     forward_actions);

    cout<<"Construction finished"<<endl;
}
