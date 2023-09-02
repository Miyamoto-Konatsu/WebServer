#include "log/log.h"
#include <thread>
#include <vector>
#include <functional>
#include <chrono>
#include <iostream>
void execTimeCompute(const std::function<void()> &func) {
    auto start_time = std::chrono::high_resolution_clock::now();

    // 调用要计时的函数
    func();

    // 获取结束时间点
    auto end_time = std::chrono::high_resolution_clock::now();

    // 计算时间差
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    // 输出秒数和毫秒数
    std::cout << "执行时间: " << duration.count() / 1000 << " 秒 "
              << duration.count() % 1000 << " 毫秒" << std::endl;
}

void func() {
    Log::getInstance()->init(0, "/root/code/cpp/TinyWebServer/build/test",
                             ".log", 1024);
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.push_back(std::thread([i]() {
            for (int j = 0; j <= 30000; ++j) {
                LOG_DEBUG("%d_%d:%s_%s", i, j, "hello", "world");
                LOG_INFO("%d_%d:%s_%s", i, j, "hello", "world");
                LOG_WARN("%d_%d:%s_%s", i, j, "hello", "world");
                LOG_ERROR("%d_%d:%s_%s", i, j, "hello", "world");
            }
        }));
    }
    for (auto &t : threads) t.join();
}
int main() {
    execTimeCompute(func);
    return 0;
}