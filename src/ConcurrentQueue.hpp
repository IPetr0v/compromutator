#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
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
class ConcurrentQueue
{
public:
    explicit ConcurrentQueue(std::shared_ptr<Alarm> alarm): alarm_(alarm) {}

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
