#pragma once

#include "../network/Graph.hpp"
#include "../network/DependencyGraph.hpp"
#include "PathScan.hpp"
#include "Timestamp.hpp"
#include "Stats.hpp"
#include "../network/Network.hpp"
#include "../network/Rule.hpp"

#include <deque>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>

struct InterceptorDiff
{
    std::vector<RulePtr> rules_to_add;
    std::vector<RulePtr> rules_to_delete;

    InterceptorDiff& operator+=(const InterceptorDiff& other);
    bool empty() {return rules_to_add.empty() && rules_to_delete.empty();}
    void clear() {
        rules_to_add.clear();
        rules_to_delete.clear();
    }
};

struct Instruction
{
    RequestList requests;
    InterceptorDiff interceptor_diff;
};

struct Prediction
{
    Prediction(RulePtr rule, Timestamp time,
               uint64_t real_counter, uint64_t predicted_counter):
        rule(rule), time(time), real_counter(real_counter),
        predicted_counter(predicted_counter) {}

    RulePtr rule;
    Timestamp time;
    uint64_t real_counter;
    uint64_t predicted_counter;
};

class FlowPredictor
{
public:
    explicit FlowPredictor(std::shared_ptr<DependencyGraph> dependency_graph,
                           std::shared_ptr<RequestIdGenerator> xid_generator);

    Instruction getInstruction();
    void passRequest(RequestPtr request);
    void updateEdges(const EdgeDiff& edge_diff);
    void predictCounter(RulePtr rule);

    std::list<Prediction> getPredictions() {return std::move(predictions_);}

    friend class Test;

private:
    std::shared_ptr<DependencyGraph> dependency_graph_;
    std::unique_ptr<PathScan> path_scan_;
    std::unique_ptr<StatsManager> stats_manager_;

    InterceptorDiff latest_interceptor_diff_;
    std::list<Prediction> predictions_;

    Timestamp current_time() const {return stats_manager_->frontTime();}

    void predict_subtree(NodeDescriptor root);

    void process_stats_list(std::list<StatsPtr>&& stats_list);
    void process_rule_query(const RuleStatsPtr& query);
    void process_path_query(const PathStatsPtr& query);
    void process_link_query(const LinkStatsPtr& query);

    bool is_in_subtree(NodeDescriptor parent, EdgeDescriptor edge) const;
    std::pair<NodeDescriptor, bool> add_child_node(NodeDescriptor parent,
                                                   EdgeDescriptor edge);
    void add_subtrees(EdgeDescriptor edge);
    void delete_subtrees(std::pair<RulePtr, RulePtr> edge);
    void add_subtree(NodeDescriptor subtree_root);
    void delete_subtree(NodeDescriptor subtree_root);

    void query_domain_path(NodeDescriptor source, NodeDescriptor sink);
    void add_domain_path(NodeDescriptor source, NodeDescriptor sink);
    void delete_domain_path(NodeDescriptor source, NodeDescriptor sink);

};

/*struct PathStats
{
    TimeId time;
    uint64_t current_flow;
    uint64_t next_flow;
};

enum class EventType
{
    START,
    QUERY,
    FINISH
};

struct LinkEvent
{
    EventType type;
    TimeId time;
    PathStats* associated_query;
};

struct LinkInterval
{
    TimeId start;
    TimeId end;
    std::map<TimeId, LinkEvent, std::less<>> event_map;
};

struct LinkTimeline
{
    LinkTimeline& getInterval(TimeId id) {
        auto it = interval_map.lower_bound(id);
        auto predesessor = --it;
        return predesessor->second;
    }

    std::map<TimeId, LinkTimeline, std::less<>> interval_map;
};

// Directed link
struct LinkNode
{
    PortPtr src_port;
    PortPtr dst_port;
    LinkTimeline timeline;
};*/
