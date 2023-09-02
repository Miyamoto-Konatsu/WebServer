#include "utils.h"
#include <chrono>

TimeNow getTimeNow() {
    TimeNow dt;
    std::chrono::system_clock::time_point now =
        std::chrono::system_clock::now();

    // 将时间点转换为C风格的时间结构
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

    // 使用C库函数localtime将时间结构分解为年、月、日、时、分、秒
    struct std::tm *parts = std::localtime(&currentTime);

    // 使用duration来获取微秒
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(
                      now.time_since_epoch())
                  % 1000000;

    // 填充结构体
    dt.year = 1900 + parts->tm_year;
    dt.month = 1 + parts->tm_mon;
    dt.day = parts->tm_mday;
    dt.hour = parts->tm_hour;
    dt.minute = parts->tm_min;
    dt.second = parts->tm_sec;
    dt.micros = micros.count();
    return dt;
}