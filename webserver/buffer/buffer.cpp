#include "buffer.h"
#include <cassert>
#include <cstddef>
#include <sys/types.h>

Buffer::Buffer(int bufferSize) : readPos_(0), writePos_(0) {
    assert(bufferSize > 0);
    buffer_.resize(bufferSize);
}

size_t Buffer::readableBytes() const {
    return writePos_ - readPos_;
}

size_t Buffer::writableBytes() const {
    return buffer_.size() - writePos_;
}

size_t Buffer::overridableBytes() const {
    return readPos_;
}

void Buffer::ensureWriteable(size_t len) {
    if (writableBytes() < len) { makeSpace(len); }
    assert(writableBytes() >= len);
}

void Buffer::hasWritten(size_t len) {
    ensureWriteable(len);
    writePos_ += len;
}

const char *Buffer::peek() const {
    return &buffer_[0] + readPos_;
}

void Buffer::retrieve(size_t len) {
    readPos_ += len;
}

void Buffer::retrieveAll() {
    readPos_ = 0;
    writePos_ = 0;
}

std::string Buffer::retrieveAllToStr() {
    std::string s(peek(), readableBytes());
    retrieveAll();
    return s;
}

const char *Buffer::beginWrite() const {
    return &buffer_[0] + writePos_;
}

char *Buffer::beginWrite() {
    return &buffer_[0] + writePos_;
}

void Buffer::append(const char *data, int len) {
    ensureWriteable(len);
    std::copy(data, data + len, beginWrite());
    hasWritten(len);
}

void Buffer::append(const void *data, int len) {
    append(static_cast<const char *>(data), len);
}

void Buffer::append(const std::string &data) {
    append(data.c_str(), data.size());
}

void Buffer::append(const Buffer &data) {
    append(data.peek(), data.readableBytes());
}

void Buffer::makeSpace(size_t len) {
    if (overridableBytes() + writableBytes() < len) {
        buffer_.resize(writePos_ + len + 1);
    } else {
        auto readable = readableBytes();
        std::copy(&buffer_[0] + readPos_, &buffer_[0] + writePos_, &buffer_[0]);
        readPos_ = 0;
        writePos_ = readable;
    }
}


Buffer::~Buffer() {
    
}