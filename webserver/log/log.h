#pragma once
#include <memory>
#include <string>
#include <mutex>
#include <thread>
#include "blockdeque.h"
#include "buffer/buffer.h"
#include <unistd.h>
#include <stdarg.h>

class Log {
public:
    void init(const int level, const char *path = "./log",
              const char *suffix = ".log", const int maxQueueCapacity = 1024);

    static Log *getInstance();

    void write(int level, const char *format, ...);

    int getLevel();
    void setLevel(int level);
    bool isOpen() {
        return isOpen_;
    }

    void flush();

private:
    void asyncWrite();

    Log() = default;
    Log(const Log &);
    Log &operator=(const Log &);
    Log(Log &&log);
    Log &operator=(Log &&log);

    ~Log();

    void writeLogLevel(int level, Buffer &buffer);

private:
    static const int LogFileNameLength = 256;
    const int MaxLineCountOneFile = 1000 * 30;

    volatile bool isOpen_;
    bool isAsync_;
    int level_;
    FILE *file_;
    const char *path_;
    const char *suffix_;
    int toDay_;
    int year_;
    int month_;
    u_int64_t lineCount_;

    std::mutex mtx_;

    std::unique_ptr<BlockDeque<std::string>> logQueue_;
    std::unique_ptr<std::thread> asyncThread_;
};

#define LOG_BASE(level, format, ...)                                           \
    Log::getInstance()->write(level, format, ##__VA_ARGS__);

#define LOG_DEBUG(format, ...) LOG_BASE(0, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) LOG_BASE(1, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) LOG_BASE(2, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) LOG_BASE(3, format, ##__VA_ARGS__)
