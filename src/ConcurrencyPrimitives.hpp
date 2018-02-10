#pragma once

#include <atomic>
#include <condition_variable>
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

    void notify() {
        alarm_.notify_one();
    }

    Status wait(std::chrono::milliseconds timeout_duration) {
        std::unique_lock<std::mutex> wait_lock(mutex_);
        auto status = alarm_.wait_for(wait_lock, timeout_duration);
        if (std::cv_status::no_timeout == status) {
            return Status::NO_TIMEOUT;
        }
        else {
            return Status::TIMEOUT;
        }
    }

private:
    std::mutex mutex_;
    std::condition_variable alarm_;

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

    void push(Item&& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        items_.push(std::move(item));
        alarm_->notify();
    }

    Item pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        Item item = std::move(items_.front());
        items_.pop();
        return std::move(item);
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
        return std::move(item);
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
