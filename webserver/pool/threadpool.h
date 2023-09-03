#pragma once

#include <iterator>
#include <queue>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <cassert>

class ThreadPool {
public:
    ThreadPool(int threadNum) : threadNum_(threadNum), isClosed_(false) {
        assert(threadNum);
    }

    void start() {
        assert(threads_.empty());
        assert(!isClosed_);

        for (int i = 0; i < threadNum_; ++i) {
            threads_.emplace_back(&ThreadPool::run, this);
        }
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mtx_);
        return tasks_.empty();
    }

    template <typename F>
    void addTask(F &&task) {
        assert(!isClosed_);
        {
            std::lock_guard<std::mutex> lock(mtx_);
            tasks_.push(std::forward<F>(task));
        }
        cv_.notify_one();
    }

    ~ThreadPool() {
        close();
    }

private:
    void run() {
        while (!isClosed_ || !empty()) {
            std::unique_lock<std::mutex> lock(mtx_);
            while (tasks_.empty()) {
                cv_.wait(lock);
                if (isClosed_ && tasks_.empty()) { return; }
            }
            auto task = std::move(tasks_.front());
            tasks_.pop();
            lock.unlock();
            task();
        }
    }

    void close() {
        isClosed_ = true;
        cv_.notify_all();
        for (auto &th : threads_) { th.join(); }
    }

private:
    volatile bool isClosed_;
    int threadNum_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
};