#pragma once
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <functional>
#include <chrono>
#include <unordered_map>
#include <vector>

using stdClock = std::chrono::high_resolution_clock;
using callbackFunc = std::function<void()>;
using TimerId = uint64_t;

class Timestamp {
public:
    Timestamp() = default;

    Timestamp(const Timestamp &other) {
        timestamp_ = other.timestamp_;
    }

    Timestamp(int seconds, int milliseconds) {
        setTime(seconds, milliseconds);
    }

    Timestamp &operator=(const Timestamp &other) {
        timestamp_ = other.timestamp_;
        return *this;
    }

    ~Timestamp() {
    }

    bool operator<(const Timestamp &other) {
        return this->timestamp_ < other.timestamp_;
    }

    // 设置时间为原时间 + seconds + milliseconds
    void addTime(int seconds, int milliseconds) {
        timestamp_ = timestamp_ + std::chrono::seconds(seconds)
                     + std::chrono::milliseconds(milliseconds);
    }

    // 设置时间为now + seconds + milliseconds
    void setTime(int seconds, int milliseconds) {
        auto now = stdClock::now();
        timestamp_ = now + std::chrono::seconds(seconds)
                     + std::chrono::milliseconds(milliseconds);
    }

    // 获取时间戳
    const stdClock::time_point &getTimestamp() const {
        return timestamp_;
    }

private:
    stdClock::time_point timestamp_;
};

class TimerNode {
public:
    TimerNode(TimerId id, const callbackFunc &cb, const Timestamp &timestamp) {
        id_ = id;
        callback_ = cb;
        timestamp_ = timestamp;
    }

    TimerNode(const TimerNode &other) {
        id_ = other.id_;
        callback_ = other.callback_;
        timestamp_ = other.timestamp_;
    }

    TimerNode &operator=(const TimerNode &other) {
        id_ = other.id_;
        callback_ = other.callback_;
        timestamp_ = other.timestamp_;
        return *this;
    }

    ~TimerNode() {
    }

    bool operator<(const TimerNode &other) {
        return this->timestamp_ < other.timestamp_;
    }

    void callback() {
        callback_();
    }

    TimerId getId() const {
        return id_;
    }

    Timestamp &getTimestamp() {
        return timestamp_;
    }

private:
    TimerId id_;
    callbackFunc callback_;
    Timestamp timestamp_;
};

class HeapTimer {
public:
    HeapTimer() {
        timers_.push_back(TimerNode(
            0, []() {}, Timestamp(0, 0)));
        timerId_ = 0;
    }
    ~HeapTimer() {
    }

    TimerId add(callbackFunc func, int seconds, int milliseconds) {
        Timestamp timestamp(seconds, milliseconds);
        addTimerNode(TimerNode(++timerId_, func, timestamp));
        return timerId_;
    }
    
    void tick() {
        while (timers_.size() > 1) {
            auto now = stdClock::now();
            auto timestamp = timers_[1].getTimestamp().getTimestamp();
            if (now < timestamp) { break; }
            if (isActivatedMap_.count(timers_[1].getId()) == 0) {
                popTimerNode();
                continue;
            }
            if (!timestampMap_.count(timers_[1].getId())) {
                timers_[1].callback();
                isActivatedMap_.erase(timers_[1].getId());
                popTimerNode();
            } else {
                timers_[1].getTimestamp() = timestampMap_[timers_[1].getId()];
                timestampMap_.erase(timers_[1].getId());
                shiftDown(1);
            }
        }
    }

    void adjust(TimerId id, int seconds, int milliseconds) {
        assert(isActivatedMap_.count(id) > 0);
        Timestamp timestamp(seconds, milliseconds);
        timestampMap_[id] = timestamp;
    }

    void delTimerNode(TimerId id) {
        isActivatedMap_.erase(id);
        timestampMap_.erase(id);
    }

    bool empty() {
        return timers_.size() == 1;
    }

    // 执行已经超时的任务并获取下一个超时时间
    int getNextTick() {
        int res = -1;
        tick();
        if (timers_.size() > 1) {
            auto now = stdClock::now();
            auto timestamp = timers_[1].getTimestamp().getTimestamp();
            if (now < timestamp) {
                res = std::chrono::duration_cast<std::chrono::milliseconds>(
                          timestamp - now)
                          .count();
                if (res < 0) { res = 0; }
            }
        }
        return res;
    }

private:
    void addTimerNode(const TimerNode &node) {
        timers_.push_back(node);
        shiftUp(timers_.size() - 1);
        isActivatedMap_[node.getId()] = true;
    }

    void popTimerNode() {
        assert(timers_.size() > 1);
        std::swap(timers_[1], timers_.back());
        timers_.pop_back();
        if (timers_.size() > 1) { shiftDown(1); }
    }

    void shiftUp(size_t index) {
        assert(index >= 1);
        while (index > 1) {
            int upIndex = index / 2;
            auto &upTimerNode = timers_[upIndex];
            auto &nowTimerNode = timers_[index];
            if (nowTimerNode < upTimerNode) {
                std::swap(upTimerNode, nowTimerNode);
                index = upIndex;
            } else {
                break;
            }
        }
    }

    void shiftDown(size_t index) {
        while (index < timers_.size() && 2 * index < timers_.size()) {
            int downIndex = 2 * index;
            if (downIndex + 1 < timers_.size()) {
                if (timers_[downIndex + 1] < timers_[downIndex]) {
                    ++downIndex;
                }
            }
            if (timers_[downIndex] < timers_[index]) {
                std::swap(timers_[downIndex], timers_[index]);
                index = downIndex;
            } else {
                break;
            }
        }
    }

private:
    std::vector<TimerNode> timers_;
    std::unordered_map<TimerId, Timestamp> timestampMap_;
    std::unordered_map<TimerId, bool> isActivatedMap_;
    TimerId timerId_;
};
