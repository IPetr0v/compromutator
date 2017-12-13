#include "Test.hpp"

void DependencyGraphTest::create_switches(SwitchList switches, PortList ports)
{
    for (auto switch_id : switches) {
        graph_.addSwitch(switch_id, ports);
    }
}

void DependencyGraphTest::create_topology(LinkList links)
{
    for (auto link : links) {
        topology_[{link.src_switch_id, link.dst_switch_id}] = link.src_port_id;
        topology_[{link.dst_switch_id, link.src_switch_id}] = link.dst_port_id;
        graph_.addLink(link.src_switch_id, link.src_port_id,
                       link.dst_switch_id, link.dst_port_id);
    }
}

RuleId DependencyGraphTest::add_forward_rule(SwitchId src_switch_id,
                                             SwitchId dst_switch_id,
                                             Priority priority,
                                             HeaderSpace header)
{
    // Define forward rule
    NetworkSpace domain;
    ActionList actions;
    actions.emplace_back(Action());
    actions[0].type = ActionType::PORT;

    // Add forward rule
    domain = NetworkSpace(1, header);
    actions[0].port_id = topology_[{src_switch_id, dst_switch_id}];
    auto rule = graph_.addRule(src_switch_id, front_table, priority,
                               domain, actions);
    return rule->id();
}

RuleId DependencyGraphTest::add_end_rule(SwitchId switch_id,
                                         Priority priority,
                                         HeaderSpace header)
{
    // Define end rule
    NetworkSpace domain;
    ActionList actions;
    actions.emplace_back(Action());
    actions[0].type = ActionType::PORT;
    actions[0].port_id = 1;

    // Add end rule
    domain = NetworkSpace(header);
    auto rule = graph_.addRule(switch_id, front_table, priority,
                               domain, actions);
    return rule->id();
}

bool DependencyGraphTest::simpleDependencyTest()
{
    SwitchList switches = {1,2,3,4};
    PortList ports = {1,2,3,4};
    LinkList links = {{1,2, 3,2},
                      {1,3, 4,3},
                      {1,4, 2,4},
                      {2,3, 3,3},
                      {2,2, 4,2},
                      {4,4, 3,4}};
    std::map<int, RuleId> rules;
    std::map<RuleId, int> inverse_rule_map;

    std::cout<<"Construct dependency graph"<<std::endl;

    // Create network
    create_switches(switches, ports);
    create_topology(links);

    // Add table miss rules
    rules[1] = graph_.tableMissRule(1, 0)->id();
    rules[2] = graph_.tableMissRule(2, 0)->id();
    rules[3] = graph_.tableMissRule(3, 0)->id();
    rules[4] = graph_.tableMissRule(4, 0)->id();

    // Add forward rules
    // 1 -> 3
    rules[5] = add_forward_rule(1, 3, 1, HeaderSpace("1111xxxx"));
    // 1 -> 4
    rules[6] = add_forward_rule(1, 4, 1, HeaderSpace("0000xxxx"));
    // 1 -> 2
    rules[7] = add_forward_rule(1, 2, 1, HeaderSpace("0011xxxx"));
    // 2 -> 3
    rules[8] = add_forward_rule(2, 3, 1, HeaderSpace("001111xx"));
    // 2 -> 4
    rules[9] = add_forward_rule(2, 4, 1, HeaderSpace("001100xx"));

    // Add end rules
    // 3 priority(3)
    rules[10] = add_end_rule(3, 3, HeaderSpace("xx11000x"));
    // 3 priority(2)
    rules[11] = add_end_rule(3, 2, HeaderSpace("xx11111x"));
    // 3 priority(1)
    rules[12] = add_end_rule(3, 1, HeaderSpace("1111xxxx"));

    // Get special rules
    rules[13] = graph_.dropRule()->id();
    rules[14] = graph_.sourceRule(1, 1)->id();
    rules[15] = graph_.sourceRule(2, 1)->id();
    rules[16] = graph_.sourceRule(3, 1)->id();
    rules[17] = graph_.sourceRule(4, 1)->id();
    rules[18] = graph_.sinkRule(3, 1)->id();

    for (const auto& rule_pair : rules) {
        inverse_rule_map[rule_pair.second] = rule_pair.first;
    }
    std::cout<<"Construction finished"<<std::endl;

    // Define anticipated dependencies
    std::map<std::pair<RuleId, RuleId>, HeaderSpace> anticipated_dependencies =
    {
        // Dependencies
        // 1 -> 3
        {{5, 10}, HeaderSpace("1111000x")},
        {{5, 11}, HeaderSpace("1111111x")},
        {{5, 12}, HeaderSpace("1111xxxx")
                  - HeaderSpace("1111000x")
                  - HeaderSpace("1111111x")},
        // 1 -> 4
        {{6, 4}, HeaderSpace("0000xxxx")},
        // 1 -> 2
        {{7, 8}, HeaderSpace("001111xx")},
        {{7, 9}, HeaderSpace("001100xx")},
        {{7, 2}, HeaderSpace("0011xxxx")
                 - HeaderSpace("001111xx")
                 - HeaderSpace("001100xx")},
        // 2 -> 3
        {{8, 11}, HeaderSpace("0011111x")},
        {{8, 3}, HeaderSpace("0011110x")},
        // 2 -> 4
        {{9, 4}, HeaderSpace("001100xx")},
        // Table dependencies
        // 1
        {{5, 1}, HeaderSpace("1111xxxx")},
        {{6, 1}, HeaderSpace("0000xxxx")},
        {{7, 1}, HeaderSpace("0011xxxx")},
        // 2
        {{8, 2}, HeaderSpace("001111xx")},
        {{9, 2}, HeaderSpace("001100xx")},
        // 3
        {{10, 12}, HeaderSpace("1111000x")},
        {{10, 3}, HeaderSpace("xx11000x")
                  - HeaderSpace("1111000x")},
        {{11, 12}, HeaderSpace("1111111x")},
        {{11, 3}, HeaderSpace("xx11111x")
                  - HeaderSpace("1111111x")},
        {{12, 3}, HeaderSpace("1111xxxx")},
        // Drop dependencies
        {{1, 13}, HeaderSpace("xxxxxxxx")},
        {{2, 13}, HeaderSpace("xxxxxxxx")},
        {{3, 13}, HeaderSpace("xxxxxxxx")},
        {{4, 13}, HeaderSpace("xxxxxxxx")},
        // Source dependencies
        // 1
        {{14, 5}, HeaderSpace("1111xxxx")},
        {{14, 6}, HeaderSpace("0000xxxx")},
        {{14, 7}, HeaderSpace("0011xxxx")},
        {{14, 1}, HeaderSpace("xxxxxxxx")
                  - HeaderSpace("1111xxxx")
                  - HeaderSpace("0000xxxx")
                  - HeaderSpace("0011xxxx")},
        // 2
        {{15, 8}, HeaderSpace("001111xx")},
        {{15, 9}, HeaderSpace("001100xx")},
        {{15, 2}, HeaderSpace("xxxxxxxx")
                  - HeaderSpace("001111xx")
                  - HeaderSpace("001100xx")},
        // 3
        {{16, 10}, HeaderSpace("xx11000x")},
        {{16, 11}, HeaderSpace("xx11111x")},
        {{16, 12}, HeaderSpace("1111xxxx")
                   - HeaderSpace("xx11000x")
                   - HeaderSpace("xx11111x")},
        {{16, 3}, HeaderSpace("xxxxxxxx")
                  - HeaderSpace("xx11000x")
                  - HeaderSpace("xx11111x")
                  - HeaderSpace("1111xxxx")},
        // 4
        {{17, 4}, HeaderSpace("xxxxxxxx")},
        // Sink dependencies
        // 3
        {{10, 18}, HeaderSpace("xx11000x")},
        {{11, 18}, HeaderSpace("xx11111x")},
        {{12, 18}, HeaderSpace("1111xxxx")},
    };

    std::cout<<"Start dependency check"<<std::endl;

    // Check dependencies
    bool dependency_check_success = true;
    auto real_dependencies = graph_.dependencies();
    for (const auto& anticipated_dependency : anticipated_dependencies) {
        RuleId src_id = anticipated_dependency.first.first;
        RuleId dst_id = anticipated_dependency.first.second;
        RuleId real_src_id = rules[src_id];
        RuleId real_dst_id = rules[dst_id];
        auto anticipated_domain = anticipated_dependency.second;

        auto it = real_dependencies.find({real_src_id, real_dst_id});
        if (it != real_dependencies.end()) {
            auto real_domain = it->second->domain.header();
            if (anticipated_domain == real_domain) {
                std::cout<<"    Correct: "
                         <<src_id<<"->"<<dst_id
                         <<" ("<<anticipated_domain<<")"<<std::endl;
            }
            else {
                dependency_check_success = false;
                std::cout<<"    Incorrect: "
                         <<src_id<<"->"<<dst_id
                         <<" ("<<anticipated_domain<<")"
                         <<" # ("<<real_domain<<")"<<std::endl;
            }
            real_dependencies.erase(it);
        }
        else {
            dependency_check_success = false;
            std::cout<<"    Missing: "
                     <<src_id<<"->"<<dst_id
                     <<" ("<<anticipated_domain<<")"<<std::endl;
        }
    }
    if (!real_dependencies.empty()) {
        dependency_check_success = false;

        for (const auto& real_dependency : real_dependencies) {
            auto real_src_id = real_dependency.first.first;
            auto real_dst_id = real_dependency.first.second;
            auto src_id = inverse_rule_map[real_src_id];
            auto dst_id = inverse_rule_map[real_dst_id];
            auto real_domain = real_dependency.second->domain.header();
            std::cout<<"    Excess: "
                     <<src_id<<"->"<<dst_id
                     <<" ["<<real_src_id<<"->"<<real_dst_id<<"]"
                     <<" ("<<real_domain<<")"<<std::endl;
        }
    }

    if (dependency_check_success)
        std::cout<<"Dependency check success"<<std::endl;
    else
        std::cout<<"Dependency check failed"<<std::endl;
}
