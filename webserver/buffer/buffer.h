#pragma once

#include <cstddef>
#include <string>
#include <sys/types.h>
#include <vector>
#include <atomic>

class Buffer {
public:
    explicit Buffer(int bufferSize = 1024);
    ~Buffer();
    
    size_t readableBytes() const;
    size_t writableBytes() const;
    size_t overridableBytes() const;

    void ensureWriteable(size_t len);
    void hasWritten(size_t len);

    const char *peek() const;
    void retrieve(size_t len);
    void retrieveAll();
    std::string retrieveAllToStr();

    const char *beginWrite() const;
    char *beginWrite();

    void append(const char *data, int len);
    void append(const void *data, int len);
    void append(const std::string &data);
    void append(const Buffer &data);

private:
    void makeSpace(size_t len);

private:
    std::vector<char> buffer_;

    size_t readPos_;
    size_t writePos_;
};