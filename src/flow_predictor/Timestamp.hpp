#pragma once

#include <chrono>

using TimestampId = uint64_t;
struct Timestamp
{
    using TimePoint = std::chrono::time_point<
        std::chrono::high_resolution_clock
    >;

    Timestamp(const Timestamp& other) = default;
    static Timestamp max() {
        return {(uint64_t)-1, TimePoint::max()};
    }

    TimestampId id;
    TimePoint value;

    bool operator<(const Timestamp& other) const {return id < other.id;}
    bool operator==(const Timestamp& other) const {return id == other.id;}
    bool operator!=(const Timestamp& other) const {return id != other.id;}

    friend class TimestampFactory;

private:
    Timestamp(uint64_t id, TimePoint value):
        id(id), value(value) {}
};

class TimestampFactory
{
public:
    TimestampFactory(): last_id_(0) {}
    Timestamp createTimestamp() {
        return {last_id_++, std::chrono::system_clock::now()};
    }

private:
    TimestampId last_id_;

};
