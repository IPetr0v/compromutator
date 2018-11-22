#include "PerformanceMonitor.hpp"

PerformanceMeasurement::PerformanceMeasurement(std::string name,
                                               ConnectionId connection_id,
                                               RequestId request_id):
    graph_size(0), network_size(0),
    start_time(std::chrono::high_resolution_clock::now()),
    name(name), id({connection_id, request_id}),
    data_filled_(false), finish_set_(false)
{

}

void PerformanceMeasurement::fill(size_t graph_size, size_t network_size)
{
    assert(not data_filled_);
    this->graph_size = graph_size;
    this->network_size = network_size;
    data_filled_ = true;
}

void PerformanceMeasurement::setFinish()
{
    assert(not finish_set_);
    duration = std::chrono::high_resolution_clock::now() - start_time;
    finish_set_ = true;
}

std::string PerformanceMeasurement::header()
{
    return "measurement,graph_size,network_size,duration,";
}

std::string PerformanceMeasurement::str() const
{
    assert(data_filled_);
    assert(finish_set_);
    auto duration_ms = std::chrono::duration_cast<
        std::chrono::milliseconds
    >(duration);
    return name + "," +
           std::to_string(graph_size) + "," +
           std::to_string(network_size) + "," +
           std::to_string(duration_ms.count()) + ",";
}

PerformanceMonitor::PerformanceMonitor(std::string measurement_filename):
    save_results_(not measurement_filename.empty()),
    measurement_filename_(measurement_filename)
{
    if (save_results_) {
        measurement_file_.open(measurement_filename_);
        measurement_file_ << PerformanceMeasurement::header() << std::endl;
    }
}

PerformanceMonitor::~PerformanceMonitor()
{
    if (save_results_) {
        measurement_file_.close();
    }
}

PerformanceMeasurementPtr
PerformanceMonitor::startMeasurement(std::string name,
                                     ConnectionId connection_id,
                                     RequestId request_id)
{
    //std::cout<<"["<<request_id<<"] start"<<std::endl;
    auto measurement = std::make_shared<PerformanceMeasurement>(
        name, connection_id, request_id
    );
    measurement_map_.emplace(measurement->id, measurement);
    return measurement;
}

void PerformanceMonitor::finishMeasurement(ConnectionId connection_id,
                                           RequestId request_id)
{
    //std::cout<<"["<<request_id<<"] finish"<<std::endl;
    auto it = measurement_map_.find({connection_id, request_id});
    if (it != measurement_map_.end()) {
        auto measurement = it->second;
        measurement->setFinish();
        measurements_to_write_.push_back(measurement);
        measurement_map_.erase(it);
    }
    else {
        auto msg = "Finishing non-existing performance measurement";
        throw std::logic_error(msg);
    }
}

void PerformanceMonitor::flush()
{
    if (save_results_) {
        for (auto measurement : measurements_to_write_) {
            auto duration_ms = std::chrono::duration_cast<
                std::chrono::milliseconds
            >(measurement->duration);
            std::cout<<"["<<measurement->id.second<<"] graph: "
                     <<measurement->graph_size<<" | net: "
                     <<measurement->network_size<<" | duration: "
                     <<std::to_string(duration_ms.count())<<std::endl;
            measurement_file_ << measurement->str() << std::endl;
        }
    }
    measurements_to_write_.clear();
}
