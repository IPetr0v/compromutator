#include "gtest/gtest.h"

#include "ExampleDependencyGraph.hpp"
#include "../src/network/DependencyGraph.hpp"
#include "../src/flow_predictor/FlowPredictor.hpp"

class StatsManagerTest : public ::testing::Test
                       , public SimpleTwoSwitchGraph
{
protected:
    virtual void SetUp() {
        initGraph();
        xid_generator = std::make_shared<RequestIdGenerator>();
        stats_manager = std::make_shared<StatsManager>(xid_generator);
    }

    virtual void TearDown() {
        stats_manager.reset();
        xid_generator.reset();
        destroyGraph();
    }

    std::shared_ptr<RequestIdGenerator> xid_generator;
    std::shared_ptr<StatsManager> stats_manager;
};

TEST_F(StatsManagerTest, BucketTest)
{
    StatsBucket bucket(xid_generator);
    auto stats = std::make_shared<RuleStats>(stats_manager->frontTime(), rule1);
    bucket.addStats(stats);
    EXPECT_TRUE(bucket.queriesExist());
    auto requests = bucket.getRequests();
    ASSERT_EQ(1u, requests.data.size());
    bucket.passRequest(requests.data[0]);
    EXPECT_TRUE(bucket.isFull());
    auto complete_stats = bucket.popStatsList();
    ASSERT_EQ(1u, complete_stats.size());
}

TEST_F(StatsManagerTest, BasicTest)
{
    uint64_t packet_count = 100u;
    PathId path_id = 1u;

    stats_manager->requestRule(rule1);
    stats_manager->requestPath(
        path_id, DomainPathDescriptor(nullptr), rule1, rule2
    );
    stats_manager->discardPathRequest(path_id);

    auto requests = stats_manager->getNewRequests();
    ASSERT_EQ(1u, requests.data.size());
    auto rule_request = RuleRequest::pointerCast(requests.data[0]);
    ASSERT_NE(nullptr, rule_request);
    EXPECT_EQ(rule1->id(), rule_request->rule->id());
    EXPECT_LT(rule_request->time.value, stats_manager->frontTime().value);
    rule_request->stats.packet_count = packet_count;
    stats_manager->passRequest(rule_request);
    auto stats_list = stats_manager->popStatsList();
    ASSERT_EQ(1u, stats_list.size());
    auto rule_stats = std::dynamic_pointer_cast<RuleStats>(*stats_list.begin());
    ASSERT_NE(nullptr, rule_stats);
    EXPECT_EQ(rule1->id(), rule_stats->rule->id());
    EXPECT_EQ(packet_count, rule_stats->stats_fields.packet_count);
}

class FlowPredictorTest : public ::testing::Test
                        , public SimpleTwoSwitchGraph
{
protected:
    using H = HeaderSpace;
    using N = NetworkSpace;

    virtual void SetUp() {
        initFlowPredictor();
        updateSpecialRuleEdges();
        updateTableMissEdges();
    }

    virtual void TearDown() {
        flow_predictor.reset();
        xid_generator.reset();
        destroyGraph();
    }

    void initFlowPredictor() {
        initGraph();
        xid_generator = std::make_shared<RequestIdGenerator>();
        flow_predictor = std::make_shared<FlowPredictor>(dependency_graph,
                                                         xid_generator);
    }

    void updateSpecialRuleEdges() {
        installSpecialRules();
        flow_predictor->updateEdges(drop_diff);
        flow_predictor->updateEdges(controller_diff);

        flow_predictor->updateEdges(p11_source_diff);
        flow_predictor->updateEdges(p11_sink_diff);
        flow_predictor->updateEdges(p12_source_diff);
        flow_predictor->updateEdges(p12_sink_diff);

        flow_predictor->updateEdges(p21_source_diff);
        flow_predictor->updateEdges(p21_sink_diff);
        flow_predictor->updateEdges(p22_source_diff);
        flow_predictor->updateEdges(p22_sink_diff);
    }

    void updateTableMissEdges() {
        installTableMissRules();
        flow_predictor->updateEdges(table_miss1_diff);
        flow_predictor->updateEdges(table_miss2_diff);
    }

    void updateFirstRuleEdges() {
        installFirstRule();
        flow_predictor->updateEdges(rule1_diff);
    }

    void updateSecondRuleEdges() {
        installSecondRule();
        flow_predictor->updateEdges(rule2_diff);
    }

    RulePtr getRule(SwitchId sw_id, PortId in_port_id,
                    const std::vector<RulePtr>& rules) {
        auto it = std::find_if(rules.begin(), rules.end(),
            [sw_id, in_port_id](RulePtr rule) {
                if (rule->sw()) {
                    return rule->sw()->id() == sw_id &&
                           rule->domain().inPort() == in_port_id;
                }
                return false;
            }
        );
        return (it != rules.end()) ? *it : nullptr;
    }

    RulePtr getRule(SwitchId sw_id, NetworkSpace domain,
                    const std::vector<RulePtr>& rules) {
        auto it = std::find_if(rules.begin(), rules.end(),
            [sw_id, domain](RulePtr rule) {
                if (rule->sw()) {
                    return rule->sw()->id() == sw_id &&
                           rule->domain() == domain;
                }
                return false;
            }
        );
        return (it != rules.end()) ? *it : nullptr;
    }

    std::shared_ptr<RequestIdGenerator> xid_generator;
    std::shared_ptr<FlowPredictor> flow_predictor;
};

TEST_F(FlowPredictorTest, CreationTest)
{
    TearDown();
    initFlowPredictor();
    updateSpecialRuleEdges();

    auto instruction = flow_predictor->getInstruction();
    EXPECT_TRUE(instruction.interceptor_diff.empty());
    EXPECT_TRUE(instruction.requests.data.empty());
}

TEST_F(FlowPredictorTest, TableMissTest)
{
    auto instruction = flow_predictor->getInstruction();
    auto& new_rules = instruction.interceptor_diff.rules_to_add;
    EXPECT_EQ(4u, new_rules.size());
    EXPECT_NE(nullptr, getRule(1u, N(1), new_rules));
    EXPECT_NE(nullptr, getRule(1u, N(2), new_rules));
    EXPECT_NE(nullptr, getRule(2u, N(1), new_rules));
    EXPECT_NE(nullptr, getRule(2u, N(2), new_rules));
    EXPECT_TRUE(instruction.interceptor_diff.rules_to_delete.empty());
    EXPECT_TRUE(instruction.requests.data.empty());
}

TEST_F(FlowPredictorTest, AddRuleTest)
{
    // Get table miss instruction
    std::cout<<"----- START -----"<<std::endl;
    std::cout<<std::endl;
    std::cout<<std::endl;
    auto table_miss_instruction = flow_predictor->getInstruction();
    auto& table_miss_new_rules =
          table_miss_instruction.interceptor_diff.rules_to_add;
    auto table_miss_source1 = getRule(1u, N(1), table_miss_new_rules);
    ASSERT_NE(nullptr, table_miss_source1);

    // Get rule1 instruction
    updateFirstRuleEdges();
    auto rule1_instruction = flow_predictor->getInstruction();

    // Check rule1 requests
    ASSERT_EQ(1u, rule1_instruction.requests.data.size());
    auto request = *rule1_instruction.requests.data.begin();
    auto rule_request = RuleRequest::pointerCast(request);
    ASSERT_NE(nullptr, rule_request);
    EXPECT_EQ(table_miss_source1->id(), rule_request->rule->id());

    // Check rule1 new rules
    auto& rule1_new_rules = rule1_instruction.interceptor_diff.rules_to_add;
    EXPECT_EQ(2u, rule1_new_rules.size());
    auto rule1_new_rule1 = getRule(1u, N(1, H("0000xxxx")), rule1_new_rules);
    auto rule1_new_rule2 = getRule(1u, N(1, H("xxxxxxxx") - H("0000xxxx")),
                                   rule1_new_rules);
    EXPECT_NE(nullptr, rule1_new_rule1);
    EXPECT_NE(nullptr, rule1_new_rule2);

    // Check rule1 deleted rules
    auto& rule1_old_rules = rule1_instruction.interceptor_diff.rules_to_delete;
    ASSERT_EQ(1u, rule1_old_rules.size());
    auto rule1_old_rule1 = getRule(1u, N(1), rule1_old_rules);
    ASSERT_NE(nullptr, rule1_old_rule1);
    EXPECT_EQ(table_miss_source1->id(), rule1_old_rule1->id());

    updateSecondRuleEdges();
    auto rule2_instruction = flow_predictor->getInstruction();
    EXPECT_EQ(1u, rule2_instruction.requests.data.size());
    EXPECT_EQ(2u, rule2_instruction.interceptor_diff.rules_to_add.size());
    EXPECT_EQ(1u, rule2_instruction.interceptor_diff.rules_to_delete.size());
}
