#include "log.h"
#include "blockdeque.h"
#include "buffer/buffer.h"
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string>
#include "utils/utils.h"
#include <iostream>

Log *Log::getInstance() {
    static Log instance;
    return &instance;
}

void Log::init(const int level, const char *path, const char *suffix,
               const int maxQueueCapacity) {
    level_ = level;
    isOpen_ = true;
    if (maxQueueCapacity > 0) {
        isAsync_ = true;
        logQueue_.reset(new BlockDeque<std::string>(maxQueueCapacity));
        asyncThread_.reset(new std::thread([this]() { asyncWrite(); }));
    } else {
        isAsync_ = false;
    }

    lineCount_ = 0;
    path_ = path;
    suffix_ = suffix;

    auto timeNow = getTimeNow();

    toDay_ = timeNow.day;
    year_ = timeNow.year;
    month_ = timeNow.month;
    char logFileName[LogFileNameLength];
    sprintf(logFileName, "%s/%04d_%02d_%02d%s", path_, year_, month_, toDay_,
            suffix_);

    file_ = fopen(logFileName, "a");
    if (nullptr == file_) {
        mkdir(path_, 644);
        file_ = fopen(logFileName, "a");
    }
    assert(file_ != nullptr);
}

void Log::write(int level, const char *format, ...) {
    auto timeNow = getTimeNow();
    {
        int oldDate = year_ * 10000 + month_ * 100 + toDay_;
        int newDate =
            (timeNow.year) * 10000 + (timeNow.month) * 100 + timeNow.day;
        std::unique_lock<std::mutex> lock(mtx_);
        ++lineCount_;
        if (oldDate < newDate || lineCount_ % MaxLineCountOneFile == 0) {
            char date[64] = {0};
            sprintf(date, "%04d_%02d_%02d", timeNow.year, timeNow.month,
                    timeNow.day);

            char logFileName[LogFileNameLength] = {0};
            if (oldDate < newDate) {
                sprintf(logFileName, "%s/%s%s", path_, date, suffix_);
                toDay_ = timeNow.day;
                year_ = timeNow.year;
                month_ = timeNow.month;
                lineCount_ = 0;
            } else if (lineCount_ && lineCount_ % MaxLineCountOneFile == 0) {
                sprintf(logFileName, "%s/%s_%d%s", path_, date,
                        int(lineCount_ / MaxLineCountOneFile), suffix_);
            }

            fflush(file_);
            fclose(file_);
            std::cout << logFileName << std::endl;
            file_ = fopen(logFileName, "a");
            assert(nullptr != file_);
        }
    }

    Buffer buffer_;
    int n = snprintf(buffer_.beginWrite(), 128,
                     "%d-%02d-%02d %02d:%02d:%02d.%06ld ", timeNow.year,
                     timeNow.month, timeNow.day, timeNow.hour, timeNow.minute,
                     timeNow.second, timeNow.micros);

    buffer_.hasWritten(n);
    writeLogLevel(level, buffer_);

    va_list ap;
    va_start(ap, format);
    int write =
        vsnprintf(buffer_.beginWrite(), buffer_.writableBytes(), format, ap);
    va_end(ap);
    buffer_.hasWritten(write);
    buffer_.append("\n\0", 2);
    auto data = buffer_.retrieveAllToStr();
    if (isAsync_ && !logQueue_->full()) {
        logQueue_->pushBack(data);
    } else {
        std::lock_guard<std::mutex> lock(mtx_);
        fputs(data.c_str(), file_);
    }
}

void Log::flush() {
    std::lock_guard<std::mutex> lock(mtx_);
    fflush(file_);
}

void Log::writeLogLevel(int level, Buffer &buffer) {
    switch (level) {
    case 0: {
        buffer.append("[debug]: ", 9);
        break;
    }
    case 1: {
        buffer.append("[info]: ", 8);
        break;
    }
    case 2: {
        buffer.append("[warn]: ", 8);
        break;
    }
    case 3: {
        buffer.append("[error]: ", 9);
        break;
    }
    default: exit(-1);
    }
}

void Log::asyncWrite() {
    std::string item;
    while (isOpen_ && logQueue_->pop(item)) {
        std::lock_guard<std::mutex> lock(mtx_);
        fputs(item.c_str(), file_);
    }
}

Log::~Log() {
    if (isAsync_) {
        while (!logQueue_->empty()) { logQueue_->notifyCons(); }
        isOpen_ = false;
        logQueue_->close();
        asyncThread_->join();
    }
    std::lock_guard<std::mutex> lock(mtx_);
    if (file_) {
        fflush(file_);
        fclose(file_);
    }
}