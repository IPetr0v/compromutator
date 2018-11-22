#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <queue>

class Alarm
{
public:
    enum class Status {
        NO_TIMEOUT,
        TIMEOUT
    };

    explicit Alarm(std::chrono::milliseconds timeout_duration):
        timeout_duration_(timeout_duration),
        remaining_time_(timeout_duration) {}

    void notify() {
        alarm_.notify_one();
    }

    //Status wait(std::chrono::milliseconds timeout_duration) {
    //    std::unique_lock<std::mutex> wait_lock(mutex_);
    //    auto status = alarm_.wait_for(wait_lock, timeout_duration);
    //    return std::cv_status::no_timeout == status
    //           ? Status::NO_TIMEOUT
    //           : Status::TIMEOUT;
    //}
    Status wait() {
        std::unique_lock<std::mutex> wait_lock(mutex_);

        auto start_time = std::chrono::high_resolution_clock::now();
        auto status = alarm_.wait_for(wait_lock, remaining_time_);
        auto finish_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<
            std::chrono::milliseconds
        >(finish_time - start_time);

        if (std::cv_status::timeout == status) {
            remaining_time_ = timeout_duration_;
            return Status::TIMEOUT;
        }
        else {
            if (duration < remaining_time_) {
                remaining_time_ -= duration;
            }
            else {
                remaining_time_ = timeout_duration_;
            }
            return Status::NO_TIMEOUT;
        }
    }

private:
    std::mutex mutex_;
    std::condition_variable alarm_;
    std::chrono::milliseconds timeout_duration_;
    std::chrono::milliseconds remaining_time_;

};

template <class Item>
class ConcurrentAlarmingQueue
{
public:
    explicit ConcurrentAlarmingQueue(std::shared_ptr<Alarm> alarm):
        alarm_(alarm) {}

    bool empty() {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_.empty();
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_.size();
    }

    void push(Item&& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        items_.push(std::move(item));
        alarm_->notify();
    }

    Item pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        Item item = std::move(items_.front());
        items_.pop();
        return item;
    }

private:
    std::mutex mutex_;
    std::shared_ptr<Alarm> alarm_;
    std::queue<Item> items_;

};

template <class Item>
class ConcurrentQueue
{
public:
    bool empty() {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_.empty();
    }

    void push(Item&& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        items_.push(std::move(item));
        non_empty_.notify_one();
    }

    Item pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (items_.empty()) {
            non_empty_.wait(lock);
        }
        Item item = std::move(items_.front());
        items_.pop();
        return item;
    }

private:
    std::mutex mutex_;
    std::condition_variable non_empty_;
    std::queue<Item> items_;

};

class Executor
{
public:
    using Task = std::function<void(void)>;

    Executor(): is_running_(true) {
        thread_ = std::thread([this]() {
            this->run();
        });
        thread_.detach();
    }

    ~Executor() {
        is_running_ = false;
        thread_.join();
    }

    void addTask(Task&& task) {
        tasks_.push(std::move(task));
    }

private:
    std::thread thread_;
    ConcurrentQueue<Task> tasks_;
    std::atomic_bool is_running_;

    void run() {
        while (is_running_) {
            auto task = tasks_.pop();
            task();
        }
    }
};
