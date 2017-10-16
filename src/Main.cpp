
#include <typeinfo>

#include <iostream>
#include <list>
#include <map>
#include <string.h>
#include <vector>

#include "Common.hpp"
#include "NetworkSpace.hpp"
#include "./detector/Detector.hpp"

int main(int argc, char* argv[])
{
    using namespace std;

    /*HeaderSpace h1("1111x01x");
    HeaderSpace h2("111101xx");
    HeaderSpace h3("1111x10x");
    HeaderSpace h4("111110xx");
    HeaderSpace h5("11111x0x");
    HeaderSpace h6("11110x1x");
    cout<<"1"<<endl;
    HeaderSpace header1 = h1|h2|h3|h4|h5|h6;

    HeaderSpace h1_("1111x01x");
    HeaderSpace h2_("111101xx");
    HeaderSpace h3_("1111x10x");
    HeaderSpace h4_("111110xx");
    HeaderSpace h5_("11111x0x");
    HeaderSpace h6_("11110x1x");
    cout<<"2"<<endl;
    HeaderSpace header2 = h1_|h4_|h5_|h2_|h3_|h6_;
    cout<<"3"<<endl;
    cout<<header1<<" - "<<header2<<" = "<<endl;
    cout<<"4"<<endl;
    cout<<header1-header2<<endl;*/

    Detector detector;
    cout<<"Construct dependency graph"<<endl;
    using ActionList = std::vector<Action>;

    // Create switches
    detector.addSwitch(1, {1,2,3,4});
    detector.addSwitch(2, {1,2,3,4});
    detector.addSwitch(3, {1,2,3,4});
    detector.addSwitch(4, {1,2,3,4});

    // Add links
    detector.addLink(1,2, 3,2);
    detector.addLink(1,3, 4,3);
    detector.addLink(1,4, 2,4);
    detector.addLink(2,3, 3,3);
    detector.addLink(2,2, 4,2);
    detector.addLink(4,4, 3,4);

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

    // Forward rules
    Priority forward_priority = 1;
    NetworkSpace forward_domain;
    ActionList forward_actions;
    forward_actions.emplace_back(Action());
    forward_actions[0].type = ActionType::PORT;

    // 1 -> 3
    forward_domain = NetworkSpace(HeaderSpace("1111xxxx"));
    forward_actions[0].port_id = 2;
    detector.addRule(1, front_table,
                     forward_priority,
                     forward_domain,
                     forward_actions);

    // 1 -> 4
    forward_domain = NetworkSpace(HeaderSpace("0000xxxx"));
    forward_actions[0].port_id = 3;
    detector.addRule(1, front_table,
                     forward_priority,
                     forward_domain,
                     forward_actions);

    // 1 -> 2
    forward_domain = NetworkSpace(HeaderSpace("0011xxxx"));
    forward_actions[0].port_id = 4;
    detector.addRule(1, front_table,
                     forward_priority,
                     forward_domain,
                     forward_actions);

    // 2 -> 3
    forward_domain = NetworkSpace(HeaderSpace("001111xx"));
    forward_actions[0].port_id = 3;
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

    // End rules
    Priority end_priority;
    NetworkSpace end_domain;
    ActionList end_actions;
    end_actions.emplace_back(Action());
    end_actions[0].type = ActionType::PORT;
    end_actions[0].port_id = 1;

    // 3 priority(3)
    end_priority = 3;
    end_domain = NetworkSpace(HeaderSpace("xx11000x"));
    detector.addRule(3, front_table,
                     end_priority,
                     end_domain,
                     end_actions);

    // 3 priority(2)
    end_priority = 2;
    end_domain = NetworkSpace(HeaderSpace("xx11111x"));
    detector.addRule(3, front_table,
                     end_priority,
                     end_domain,
                     end_actions);

    // 3 priority(1)
    end_priority = 1;
    end_domain = NetworkSpace(HeaderSpace("1111xxxx"));
    detector.addRule(3, front_table,
                     end_priority,
                     end_domain,
                     end_actions);

    cout<<"Construction finished"<<endl;
}
