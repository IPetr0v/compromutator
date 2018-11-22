#pragma once

#include "Types.hpp"
#include "flow_predictor/Timestamp.hpp"

#include <fstream>
#include <map>

struct PerformanceMeasurement {
    using Id = std::pair<ConnectionId, RequestId>;

    explicit PerformanceMeasurement(std::string name,
                                    ConnectionId connection_id,
                                    RequestId request_id);

    void fill(size_t graph_size, size_t network_size);
    void setFinish();

    static std::string header();
    // TODO: Use operator<<
    std::string str() const;

    size_t graph_size;
    size_t network_size;
    Timestamp::TimePoint start_time;
    Timestamp::Duration duration;

    std::string name;
    Id id;

private:
    bool data_filled_;
    bool finish_set_;
};
using PerformanceMeasurementPtr = std::shared_ptr<PerformanceMeasurement>;

class PerformanceMonitor {
public:
    // TODO: Do not measure if perf file is not specified !!!
    explicit PerformanceMonitor(std::string measurement_filename);
    ~PerformanceMonitor();

    PerformanceMeasurementPtr startMeasurement(std::string name,
                                               ConnectionId connection_id,
                                               RequestId request_id);
    void finishMeasurement(ConnectionId connection_id, RequestId request_id);
    void flush();

private:
    bool save_results_;
    std::string measurement_filename_;
    std::ofstream measurement_file_;
    std::map<PerformanceMeasurement::Id,
             PerformanceMeasurementPtr> measurement_map_;
    std::list<PerformanceMeasurementPtr> measurements_to_write_;

};
