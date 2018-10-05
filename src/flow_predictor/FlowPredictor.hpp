#pragma once

#include "../network/Graph.hpp"
#include "../network/DependencyGraph.hpp"
#include "PathScan.hpp"
#include "InterceptorManager.hpp"
#include "Timestamp.hpp"
#include "Stats.hpp"
#include "../network/Network.hpp"
#include "../network/Rule.hpp"

#include <deque>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>

struct Prediction
{
    Prediction(RequestId request_id, RulePtr rule):
        request_id(request_id), rule(rule), real_counter({0, 0}),
        predicted_counter({0, 0}) {}
    void update(RuleStatsFields real, RuleStatsFields predicted);

    RequestId request_id;
    RulePtr rule;
    RuleStatsFields real_counter;
    RuleStatsFields predicted_counter;
};
using PredictionList = std::list<Prediction>;

struct Instruction
{
    RequestList requests;
    RuleReplyList replies;
    InterceptorDiff interceptor_diff;

    bool empty() const {
        return requests.data.empty() &&
               replies.empty() &&
               interceptor_diff.empty();
    }
};

class FlowPredictor
{
public:
    explicit FlowPredictor(std::shared_ptr<DependencyGraph> dependency_graph,
                           std::shared_ptr<RequestIdGenerator> xid_generator);

    Instruction getInstruction();
    void passRequest(RequestPtr request);
    void updateEdges(const EdgeDiff& edge_diff);
    void predictFlow(RequestId request_id, std::list<RulePtr> rules);

private:
    std::shared_ptr<DependencyGraph> dependency_graph_;
    std::unique_ptr<PathScan> path_scan_;
    std::unique_ptr<InterceptorManager> interceptor_manager_;
    std::unique_ptr<StatsManager> stats_manager_;

    InterceptorDiff latest_interceptor_diff_;
    PredictionList predictions_;
    std::unordered_map<TimestampId, PredictionList> pending_predictions_;

    Timestamp current_time() const {return stats_manager_->frontTime();}

    void predict_subtree(NodePtr root);
    void update_predictions(TimestampId timestamp);
    RuleReplyList create_replies();

    void process_stats_list(std::list<StatsPtr>&& stats_list);
    void process_rule_query(const RuleStatsPtr& query);
    void process_path_query(const PathStatsPtr& query);
    void process_link_query(const LinkStatsPtr& query);

    bool is_existing_child(NodePtr parent, EdgePtr edge) const;
    std::pair<NodePtr, bool> add_child_node(NodePtr parent,
                                                   EdgePtr edge);
    void add_subtrees(EdgePtr edge);
    void delete_subtrees(std::pair<RulePtr, RulePtr> edge);
    void add_subtree(NodePtr subtree_root);
    void delete_subtree(NodePtr subtree_root);

    void query_domain_path(NodePtr source, NodePtr sink);
    void add_domain_path(NodePtr source, NodePtr sink);
    void delete_domain_path(NodePtr source, NodePtr sink);
};
