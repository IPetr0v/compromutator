#include "gtest/gtest.h"

#include "ExampleNetwork.hpp"
#include "../src/NetworkSpace.hpp"
#include "../src/network/Network.hpp"

#include <memory>
#include <set>
#include <vector>

TEST(InitSwitchTest, CreationTest)
{
    std::vector<PortInfo> ports{{1,0}, {2,0}};
    auto sw = new Switch(SwitchInfo(1, 3, ports));
    ASSERT_NE(nullptr, sw);
    ASSERT_NE(sw->tables().begin(), sw->tables().end());
    auto table = *sw->tables().begin();
    ASSERT_NE(nullptr, table);
    EXPECT_TRUE(sw->isFrontTable(table));
    delete sw;
}

class SwitchTest : public ::testing::Test
{
protected:
    using H = HeaderSpace;
    using N = NetworkSpace;

    virtual void SetUp() {
        std::vector<PortInfo> ports{{1,0}, {2,0}};
        sw = new Switch(SwitchInfo(1, 3, ports));
    }

    virtual void TearDown() {
        delete sw;
    }

    SwitchPtr sw;
    std::vector<PortId> ports{1,2};
};

TEST_F(SwitchTest, PortTest)
{
    uint32_t port_number = 1;
    EXPECT_NE(nullptr, sw->port(port_number));
    ASSERT_NE(sw->tables().begin(), sw->tables().end());
    for (auto port : sw->ports()) {
        EXPECT_EQ(port_number, port->id());

        auto source_rule = port->sourceRule();
        ASSERT_NE(nullptr, source_rule);
        EXPECT_EQ(N(port_number), source_rule->domain());
        const auto& source_actions = source_rule->actions();
        EXPECT_TRUE(source_actions.port_actions.empty());

        auto sink_rule = port->sinkRule();
        ASSERT_NE(nullptr, sink_rule);
        EXPECT_EQ(port_number, sink_rule->inPort());
        EXPECT_EQ(N(port_number), sink_rule->domain());
        const auto& sink_actions = sink_rule->actions();
        EXPECT_TRUE(sink_actions.port_actions.empty());

        port_number++;
    }
    EXPECT_EQ(ports.size(), port_number - 1);
}

TEST_F(SwitchTest, TableTest)
{
    auto table = sw->table(0);
    ASSERT_NE(nullptr, table);
    EXPECT_EQ((*sw->tables().begin())->id(), table->id());
    auto table_miss_rule = table->tableMissRule();
    ASSERT_NE(table->rules().begin(), table->rules().end());
    auto rule = *table->rules().begin();
    ASSERT_NE(nullptr, rule);
    EXPECT_EQ(table_miss_rule->id(), rule->id());
    EXPECT_EQ(NetworkSpace::wholeSpace(), table_miss_rule->domain());
}

TEST(BasicNetworkTest, CreationTest)
{
    auto network = new Network();
    ASSERT_NE(nullptr, network);
    std::vector<PortInfo> ports{{1,0}, {2,0}};
    auto sw = network->addSwitch(SwitchInfo(1, 2, ports));
    ASSERT_NE(nullptr, network->getSwitch(1));
    EXPECT_EQ(sw->id(), network->getSwitch(1)->id());
    delete network;
}

class NetworkTest : public ::testing::Test
                  , public TwoSwitchNetwork
{
protected:
    virtual void SetUp() {
        initNetwork();
        new_rule0 = network->addRule(1, 0, 3, 0x0, N(1, H("00001111")),
                                     ActionsBase::portAction(2));
        deleted_rule5_id_ = rule5->id();
        network->deleteRule(deleted_rule5_id_);
    }

    virtual void TearDown() {
        destroyNetwork();
    }

    RulePtr new_rule0;
    RuleId deleted_rule5_id_;
};

TEST_F(NetworkTest, LinkTest)
{
    auto link_pair = network->link({1,2}, {2,1});
    bool success = link_pair.second;
    EXPECT_TRUE(success);
    auto link = link_pair.first;
    ASSERT_NE(nullptr, link.src_port);
    ASSERT_NE(nullptr, link.dst_port);
    EXPECT_EQ(port12->id(), link.src_port->id());
    EXPECT_EQ(port21->id(), link.dst_port->id());
    EXPECT_EQ(port12->id(), network->adjacentPort(link.dst_port)->id());
    EXPECT_EQ(port21->id(), network->adjacentPort(link.src_port)->id());

    network->deleteLink({1,2}, {2,1});
    bool deleted_link_found = network->link({1,2}, {2,1}).second;
    EXPECT_FALSE(deleted_link_found);
    EXPECT_EQ(nullptr, network->adjacentPort(link.dst_port));
    EXPECT_EQ(nullptr, network->adjacentPort(link.src_port));
}

TEST_F(NetworkTest, TableRuleTest)
{
    auto found_rule = sw1->table(1)->rule(rule11->id());
    ASSERT_NE(nullptr, found_rule);
    EXPECT_EQ(rule11->id(), found_rule->id());

    auto deleted_rule = sw1->table(0)->rule(deleted_rule5_id_);
    EXPECT_EQ(nullptr, deleted_rule);

    auto rules_lower_than_2 = sw1->table(0)->lowerRules(rule2);
    ASSERT_NE(rules_lower_than_2.begin(), rules_lower_than_2.end());
    std::set<RulePtr, Rule::PtrComparator> lower_rule_set{table_miss};
    for (auto rule : rules_lower_than_2) {
        auto it = lower_rule_set.find(rule);
        EXPECT_NE(lower_rule_set.end(), it);
        lower_rule_set.erase(it);
    }
    EXPECT_TRUE(lower_rule_set.empty());

    auto rules_upper_than_4 = sw1->table(0)->upperRules(rule4);
    ASSERT_NE(rules_upper_than_4.begin(), rules_upper_than_4.end());
    std::set<RulePtr, Rule::PtrComparator> upper_rule_set{new_rule0, rule1};
    for (auto rule : rules_upper_than_4) {
        auto it = upper_rule_set.find(rule);
        EXPECT_NE(upper_rule_set.end(), it);
        upper_rule_set.erase(it);
    }
    EXPECT_TRUE(upper_rule_set.empty());
}

TEST_F(NetworkTest, PortRuleTest)
{
    auto rules_from_port11 = port11->dstRules();
    ASSERT_NE(rules_from_port11.begin(), rules_from_port11.end());
    std::set<RulePtr, Rule::PtrComparator> expected_port11_rules{
        new_rule0, rule1, rule2, rule3, rule4, table_miss
    };
    for (auto rule : rules_from_port11) {
        auto it = expected_port11_rules.find(rule);
        EXPECT_NE(expected_port11_rules.end(), it);
        expected_port11_rules.erase(it);
    }
    EXPECT_TRUE(expected_port11_rules.empty());

    auto rules_to_port12 = port12->srcRules();
    ASSERT_NE(rules_to_port12.begin(), rules_to_port12.end());
    std::set<RulePtr, Rule::PtrComparator> expected_port12_rules{
        new_rule0, rule2, rule3, rule11
    };
    for (auto rule : rules_to_port12) {
        auto it = expected_port12_rules.find(rule);
        EXPECT_NE(expected_port12_rules.end(), it);
        expected_port12_rules.erase(it);
    }
    EXPECT_TRUE(expected_port12_rules.empty());

    network->deleteRule(new_rule0->id());
    network->deleteRule(rule1->id());
    network->deleteRule(rule2->id());
    network->deleteRule(rule3->id());
    network->deleteRule(rule4->id());
    EXPECT_FALSE(port11->srcRules().begin() != port11->srcRules().end());
}
