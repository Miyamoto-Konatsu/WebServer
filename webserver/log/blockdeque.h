#pragma once

#include <chrono>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <type_traits>

template <typename T>
class BlockDeque {
public:
    explicit BlockDeque(size_t maxCapacity = 1024) : capacity_(maxCapacity) {
    }

    ~BlockDeque() {
        close();
    }

    bool full() {
        std::lock_guard<std::mutex> lock(mtx_);
        return deque_.size() >= capacity_;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mtx_);
        return deque_.empty();
    }

    void notifyCons() {
        condConsumer_.notify_all();
    }

    void pushBack(const T &item) {
        std::unique_lock<std::mutex> lock(mtx_);
        while (deque_.size() >= capacity_) { condProducer_.wait(lock); }
        deque_.push_back(item);
        condConsumer_.notify_one();
    }

    void pushFront(const T &item) {
        std::unique_lock<std::mutex> lock(mtx_);
        while (deque_.size() >= capacity_) { condProducer_.wait(lock); }
        deque_.push_front(item);
        condConsumer_.notify_one();
    }

    bool pop(T &item) {
        {
            std::unique_lock<std::mutex> lock(mtx_);
            while (deque_.empty()) {
                condConsumer_.wait(lock);
                if (isClose_) { return false; }
            }
            item = std::move(deque_.back());
            deque_.pop_back();
        }
        condProducer_.notify_one();
        return true;
    }

    bool pop(T &item, int timeOutMs) {
        {
            std::unique_lock<std::mutex> lock(mtx_);
            while (deque_.empty()) {
                auto status = condConsumer_.wait_for(
                    lock, std::chrono::milliseconds(timeOutMs));
                if (status == std::cv_status::timeout) { return false; }
                if (isClose_) { return false; }
            }
            item = std::move(deque_.front());
            deque_.pop_front();
        }
        condProducer_.notify_one();
        return true;
    }

    void close() {
        {
            std::unique_lock<std::mutex> lock(mtx_);
            isClose_ = true;
            deque_.clear();
        }
        condProducer_.notify_all();
        condConsumer_.notify_all();
    }

private:
    std::deque<T> deque_;
    const int capacity_;
    std::mutex mtx_;

    std::condition_variable condConsumer_;
    std::condition_variable condProducer_;
    volatile bool isClose_;
};