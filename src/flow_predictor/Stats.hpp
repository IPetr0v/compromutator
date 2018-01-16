#pragma once

#include "PathScan.hpp"
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
    PathStats(Timestamp time, DomainPathDescriptor path,
              RulePtr source_interceptor, RulePtr sink_interceptor):
        Stats(time), path(path),
        source_interceptor(source_interceptor),
        sink_interceptor(sink_interceptor) {}

    DomainPathDescriptor path;
    RulePtr source_interceptor;
    RulePtr sink_interceptor;

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
    RuleRequest(RequestId id, RequestType type, Timestamp time, RulePtr rule):
        Request(id, time, type), rule(rule)
    {
        assert(type == RequestType::RULE ||
               type == RequestType::SOURCE_RULE ||
               type == RequestType::SINK_RULE);
    };

    RulePtr rule;
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
                        Timestamp time, RulePtr rule)
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

class StatsBucket
{
public:
    StatsBucket(std::shared_ptr<RequestIdGenerator> xid_generator):
        xid_generator_(xid_generator) {}

    void addStats(StatsPtr stats);

    RequestList getRequests();
    void passRequest(RequestPtr request);

    std::list<StatsPtr> popStatsList() {return std::move(stats_list_);}
    bool isFull() const {return expected_requests_.empty();}
    bool queriesExist() const {return not stats_list_.empty();}

private:
    std::list<StatsPtr> stats_list_;
    std::map<RequestId, StatsPtr> expected_requests_;
    std::shared_ptr<RequestIdGenerator> xid_generator_;

    StatsPtr get_stats(RequestId id);
    void pass_rule_request(RuleRequestPtr request);
    void pass_port_request(PortRequestPtr request);

};

class StatsManager
{
    enum class Position {FRONT, BACK};
public:
    explicit StatsManager(std::shared_ptr<RequestIdGenerator> xid_generator);

    Timestamp frontTime() const;
    Timestamp backTime() const;

    void requestRule(RulePtr rule);
    void requestPath(DomainPathDescriptor path,
                     RulePtr source_interceptor,
                     RulePtr sink_interceptor);
    void requestLink(PortPtr src_port, PortPtr dst_port);
    // TODO: delete requests

    RequestList getNewRequests();
    void passRequest(RequestPtr request);

    std::list<StatsPtr> popStatsList();

private:
    std::unordered_map<TimestampId, StatsBucket> stats_timeline_;
    std::deque<Timestamp> timestamp_deque_;
    TimestampFactory timestamp_factory_;
    std::shared_ptr<RequestIdGenerator> xid_generator_;

    StatsBucket& get_bucket(Timestamp time);
    StatsBucket& get_bucket(Position pos);
    void add_front_bucket();
    void delete_back_bucket();

};

