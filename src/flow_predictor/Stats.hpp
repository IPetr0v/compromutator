#pragma once

#include "PathScan.hpp"
#include "InterceptorManager.hpp"
#include "Timestamp.hpp"
#include "../network/Rule.hpp"

#include <deque>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>

struct Stats
{
    Stats(Timestamp time): time(time) {}
    virtual ~Stats() = default;

    Timestamp time;
};
using StatsPtr = std::shared_ptr<Stats>;

struct RuleStats : public Stats
{
    RuleStats(Timestamp time, RulePtr rule):
        Stats(time), rule(rule) {}

    RulePtr rule;
    RuleStatsFields stats_fields;
};
using RuleStatsPtr = std::shared_ptr<RuleStats>;

struct PathStats : public Stats
{
    PathStats(Timestamp time, DomainPathPtr path):
        Stats(time), path(path) {}

    DomainPathPtr path;

    RuleStatsFields source_stats_fields;
    RuleStatsFields sink_stats_fields;
};
using PathStatsPtr = std::shared_ptr<PathStats>;

struct LinkStats : public Stats
{
    LinkStats(Timestamp time, PortPtr src_port, PortPtr dst_port):
        Stats(time), src_port(src_port), dst_port(dst_port) {}

    PortPtr src_port;
    PortPtr dst_port;

    PortStatsFields src_stats_fields;
    PortStatsFields dst_stats_fields;
};
using LinkStatsPtr = std::shared_ptr<LinkStats>;

enum class RequestType
{
    RULE, SOURCE_RULE, SINK_RULE, SRC_PORT, DST_PORT
};

struct Request
{
    Request(RequestId id, Timestamp time, RequestType type):
        id(id), time(time), type(type) {}
    virtual ~Request() = default;

    RequestId id;
    Timestamp time;
    RequestType type;
};
using RequestPtr = std::shared_ptr<Request>;

struct RuleRequest;
using RuleRequestPtr = std::shared_ptr<RuleRequest>;
struct RuleRequest : public Request
{
    RuleRequest(RequestId id, RequestType type, Timestamp time,
                RuleInfoPtr rule):
        Request(id, time, type), rule(rule)
    {
        assert(type == RequestType::RULE ||
               type == RequestType::SOURCE_RULE ||
               type == RequestType::SINK_RULE);
    };

    RuleInfoPtr rule;
    RuleStatsFields stats;

    static RuleRequestPtr pointerCast(RequestPtr request) {
        return std::dynamic_pointer_cast<RuleRequest>(request);
    }
};

struct PortRequest;
using PortRequestPtr = std::shared_ptr<PortRequest>;
struct PortRequest : public Request
{
    PortRequest(RequestId id, RequestType type, Timestamp time, PortPtr port):
        Request(id, time, type), port(port)
    {
        assert(type == RequestType::SRC_PORT || type == RequestType::DST_PORT);
    };

    PortPtr port;
    PortStatsFields stats;

    static PortRequestPtr pointerCast(RequestPtr request) {
        return std::dynamic_pointer_cast<PortRequest>(request);
    }
};

struct RequestList
{
    void addRuleRequest(RequestId id, RequestType type,
                        Timestamp time, RuleInfoPtr rule)
    {
        auto rule_request = std::make_shared<RuleRequest>(
            id, type, time, rule
        );
        data.emplace_back(rule_request);
    }
    void addPortRequest(RequestId id, RequestType type,
                        Timestamp time, PortPtr port)
    {
        auto port_request = std::make_shared<PortRequest>(
            id, type, time, port
        );
        data.emplace_back(port_request);
    }

    std::vector<RequestPtr> data;
};

struct Reply
{
    Reply(RequestId request_id, SwitchId switch_id):
        request_id(request_id), switch_id(switch_id) {}
    RequestId request_id;
    SwitchId switch_id;
};

struct RuleReply : public Reply
{
    RuleReply(RequestId request_id, SwitchId switch_id):
        Reply(request_id, switch_id) {}

    struct Flow {
        RuleInfoPtr rule;
        RuleStatsFields stats;
        //Timestamp::Duration duration;
    };
    std::list<Flow> flows;

    void addFlow(RuleInfoPtr rule, RuleStatsFields stats) {
                 //Timestamp::Duration duration) {
        //flows.push_back(Flow{std::move(rule), stats, duration});
        flows.push_back(Flow{std::move(rule), stats});
    }
};
using RuleReplyPtr = std::shared_ptr<RuleReply>;
using RuleReplyList = std::list<RuleReplyPtr>;

using StatsDescriptor = std::list<StatsPtr>::iterator;
class StatsBucket
{
public:
    StatsBucket(std::shared_ptr<RequestIdGenerator> xid_generator):
        xid_generator_(xid_generator) {}

    StatsDescriptor addStats(StatsPtr stats);
    void deleteStats(StatsDescriptor stats_desc);

    RequestList getRequests();
    void passRequest(RequestPtr request);

    std::list<StatsPtr> popStatsList() {return std::move(stats_list_);}
    bool isFull() const {return expected_requests_.empty();}
    bool queriesExist() const {return not stats_list_.empty();}

private:
    std::list<StatsPtr> stats_list_;
    // These requests have been sent to the network
    std::map<RequestId, StatsPtr> expected_requests_;
    std::shared_ptr<RequestIdGenerator> xid_generator_;

    StatsPtr get_stats(RequestId id);
    void pass_rule_request(RuleRequestPtr request);
    void pass_port_request(PortRequestPtr request);
};
using StatsBucketPtr = std::shared_ptr<StatsBucket>;

struct StatsList
{
    TimestampId timestamp;
    std::list<StatsPtr> stats;
};

class StatsManager
{
    enum class Position {FRONT, BACK};
public:
    explicit StatsManager(std::shared_ptr<RequestIdGenerator> xid_generator);

    Timestamp frontTime() const;
    Timestamp backTime() const;

    void requestRule(RulePtr rule);
    void requestPath(DomainPathPtr path);
    void requestLink(PortPtr src_port, PortPtr dst_port);
    void discardPathRequest(DomainPathPtr path);

    RequestList getNewRequests();
    void passRequest(RequestPtr request);

    StatsList popStatsList();

private:
    std::unordered_map<TimestampId, StatsBucketPtr> stats_timeline_;
    std::deque<Timestamp> timestamp_deque_;
    TimestampFactory timestamp_factory_;
    std::shared_ptr<RequestIdGenerator> xid_generator_;

    // This map is used to quickly delete path stats that have not been sent
    // to the network.
    std::map<PathId, StatsDescriptor> current_path_stats_;

    StatsBucketPtr get_bucket(Timestamp time);
    StatsBucketPtr get_bucket(Position pos);
    void add_front_bucket();
    void delete_bucket(Position pos);

};

