#include "Stats.hpp"

#include <memory>

void StatsBucket::addStats(StatsPtr stats)
{
    stats_list_.push_back(stats);
}

RequestList StatsBucket::getRequests()
{
    RequestList requests;
    for (const auto& stats : stats_list_) {
        auto time = stats->time;
        if (auto rule_stats = std::dynamic_pointer_cast<RuleStats>(stats))
        {
            auto id = xid_generator_->getId();
            requests.addRuleRequest(id, RequestType::RULE, time,
                                    rule_stats->rule);
            expected_requests_.emplace(id, rule_stats);
        }
        else if (auto path_stats = std::dynamic_pointer_cast<PathStats>(stats))
        {
            auto src_id = xid_generator_->getId();
            auto dst_id = xid_generator_->getId();
            requests.addRuleRequest(src_id, RequestType::SOURCE_RULE, time,
                                    path_stats->source_interceptor);
            requests.addRuleRequest(dst_id, RequestType::SINK_RULE, time,
                                    path_stats->sink_interceptor);
            expected_requests_.emplace(src_id, path_stats);
            expected_requests_.emplace(dst_id, path_stats);
        }
        else if (auto link_stats = std::dynamic_pointer_cast<LinkStats>(stats))
        {
            auto src_id = xid_generator_->getId();
            auto dst_id = xid_generator_->getId();
            requests.addPortRequest(src_id, RequestType::SRC_PORT, time,
                                    link_stats->src_port);
            requests.addPortRequest(dst_id, RequestType::DST_PORT, time,
                                    link_stats->dst_port);
            expected_requests_.emplace(src_id, link_stats);
            expected_requests_.emplace(dst_id, link_stats);
        }
        else {
            assert(0);
        }
    }
    return requests;
}

void StatsBucket::passRequest(RequestPtr request)
{
    if (auto rule_request = RuleRequest::pointerCast(request)) {
        pass_rule_request(rule_request);
    }
    else if (auto port_request = PortRequest::pointerCast(request)) {
        pass_port_request(port_request);
    }
    else {
        assert(0);
    }
}

StatsPtr StatsBucket::get_stats(RequestId id)
{
    auto it = expected_requests_.find(id);
    assert(it != expected_requests_.end());
    StatsPtr stats = it->second;
    expected_requests_.erase(it);
    return stats;
}

void StatsBucket::pass_rule_request(RuleRequestPtr request)
{
    auto stats = get_stats(request->id);
    if (auto rule_stats = std::dynamic_pointer_cast<RuleStats>(stats)) {
        assert(request->type == RequestType::RULE);
        rule_stats->stats_fields = request->stats;
    }
    else if (auto path_stats = std::dynamic_pointer_cast<PathStats>(stats)) {
        switch (request->type) {
        case RequestType::SOURCE_RULE:
            path_stats->source_stats_fields = request->stats;
            break;
        case RequestType::SINK_RULE:
            path_stats->sink_stats_fields = request->stats;
            break;
        default:
            assert(0);
        }
    }
    else {
        assert(0);
    }
}

void StatsBucket::pass_port_request(PortRequestPtr request)
{
    auto stats = get_stats(request->id);
    if (auto link_stats = std::dynamic_pointer_cast<LinkStats>(stats)) {
        switch (request->type) {
        case RequestType::SRC_PORT:
            link_stats->src_stats_fields = request->stats;
            break;
        case RequestType::DST_PORT:
            link_stats->dst_stats_fields = request->stats;
            break;
        default:
            assert(0);
        }
    }
    else {
        assert(0);
    }
}

StatsManager::StatsManager(std::shared_ptr<RequestIdGenerator> xid_generator):
    xid_generator_(xid_generator)
{
    add_front_bucket();
}

Timestamp StatsManager::frontTime() const
{
    assert(not timestamp_deque_.empty());
    return timestamp_deque_.front();
}

Timestamp StatsManager::backTime() const
{
    assert(not timestamp_deque_.empty());
    return timestamp_deque_.back();
}

void StatsManager::requestRule(RulePtr rule)
{
    auto time = frontTime();
    auto bucket = get_bucket(Position::FRONT);
    auto stats = std::make_shared<RuleStats>(time, rule);
    bucket.addStats(stats);
}

void StatsManager::requestPath(DomainPathDescriptor path,
                               RulePtr source_interceptor,
                               RulePtr sink_interceptor)
{
    auto time = frontTime();
    auto bucket = get_bucket(Position::FRONT);
    auto stats = std::make_shared<PathStats>(time, path,
                                             source_interceptor,
                                             sink_interceptor);
    bucket.addStats(stats);
}

void StatsManager::requestLink(PortPtr src_port, PortPtr dst_port)
{
    auto time = frontTime();
    auto bucket = get_bucket(Position::FRONT);
    auto stats = std::make_shared<LinkStats>(time, src_port, dst_port);
    bucket.addStats(stats);
}

RequestList StatsManager::getNewRequests()
{
    auto bucket = get_bucket(Position::FRONT);
    if (bucket.queriesExist()) {
        auto requests = bucket.getRequests();
        add_front_bucket();
        return std::move(requests);
    }
    else {
        return RequestList();
    }
}

void StatsManager::passRequest(RequestPtr request)
{
    auto bucket = get_bucket(request->time);
    bucket.passRequest(request);
}

std::list<StatsPtr> StatsManager::popStatsList()
{
    // Get queries
    auto bucket = get_bucket(Position::BACK);
    if (bucket.isFull()) {
        auto queries = bucket.popStatsList();
        delete_back_bucket();
        return std::move(queries);
    }
    else {
        return std::list<StatsPtr>();
    }
}

StatsBucket& StatsManager::get_bucket(Timestamp time)
{
    auto bucket_it = stats_timeline_.find(time.id);
    assert(bucket_it != stats_timeline_.end());
    auto& bucket = bucket_it->second;
    return bucket;
}

StatsBucket& StatsManager::get_bucket(Position pos)
{
    auto time = (pos == Position::FRONT) ? frontTime() : backTime();
    return get_bucket(time);
}

void StatsManager::add_front_bucket()
{
    auto time = timestamp_factory_.createTimestamp();
    timestamp_deque_.push_front(time);
    stats_timeline_.emplace(time.id, StatsBucket(xid_generator_));
}

void StatsManager::delete_back_bucket()
{
    auto time = backTime();
    auto bucket_it = stats_timeline_.find(time.id);
    assert(bucket_it != stats_timeline_.end());
    stats_timeline_.erase(bucket_it);
    timestamp_deque_.pop_back();
}
