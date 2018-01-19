#include "gtest/gtest.h"

#include "ExampleNetwork.hpp"
#include "../src/NetworkSpace.hpp"
#include "../src/network/Network.hpp"

#include <memory>
#include <set>
#include <vector>

TEST(BasicSwitchTest, CreationTest)
{
    auto sw = new Switch(1, {1,2}, 3);
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
        sw = new Switch(1, ports, 3);
    }

    virtual void TearDown() {
        delete sw;
    }

    SwitchPtr sw;
    std::vector<PortId> ports{1,2};
};

TEST_F(SwitchTest, PortTest)
{
    int port_number = 1;
    EXPECT_NE(nullptr, sw->port(port_number));
    ASSERT_NE(sw->tables().begin(), sw->tables().end());
    for (auto port : sw->ports()) {
        EXPECT_EQ(port_number, port->id());

        auto source_rule = port->sourceRule();
        ASSERT_NE(nullptr, source_rule);
        EXPECT_EQ(N::wholeSpace(), source_rule->domain());
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
    auto sw = network->addSwitch(1, {1,2}, 2);
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
        new_rule0 = network->addRule(1, 0, 3, N(1, H("00001111")),
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
}
