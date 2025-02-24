#pragma once
#include <stdexcept>
#include <vector>
#include <span>
#include <cstring>
#include "swap.h"

namespace MmtTlv {
    
namespace Common {

class ReadStream final {
public:
    explicit ReadStream(const std::vector<uint8_t>& data);
    explicit ReadStream(const std::vector<uint8_t>& data, uint32_t size);
    explicit ReadStream(ReadStream& stream, uint32_t size);
    explicit ReadStream(ReadStream& stream);

    ReadStream(const ReadStream&) = delete;
    ReadStream& operator=(const ReadStream&) = delete;

    ReadStream(ReadStream&&) = default;
    ReadStream& operator=(ReadStream&&) = default;

    bool isEof() const { return size == cur; }
    size_t leftBytes() const { return size - cur; }
    size_t getCur() const { return cur; }

    void setCur(size_t cur) {
        if (size < cur) {
            throw std::out_of_range("Access out of bounds");
        }
        this->cur = cur;
    }

    void skip(uint64_t pos) {
        if (size < cur + pos) {
            throw std::out_of_range("Access out of bounds");
        }
        cur += pos;
    }

    size_t read(void* dst, size_t size) {
        size_t readBytes = peek(dst, size);
        cur += readBytes;
        return readBytes;
    }

    size_t read(std::span<uint8_t> data) {
        size_t readBytes = peek(data);
        cur += readBytes;
        return readBytes;
    }

    size_t peek(void* dst, size_t size) {
        if (this->size < cur + size) {
            throw std::out_of_range("Access out of bounds");
        }

        memcpy(dst, buffer.data() + cur, size);
        return size;
    }

    size_t peek(std::span<uint8_t> data) {
        if (this->size < cur + size) {
            throw std::out_of_range("Access out of bounds");
        }

        std::copy(buffer.begin() + cur, buffer.begin() + cur + buffer.size(), data.begin());
        return size;
    }

    uint8_t get8U() {
        uint8_t value;
        read(&value, sizeof(value));
        return value;
    }

    uint16_t getBe16U() {
        uint16_t value;
        read(&value, sizeof(value));
        return swapEndian16(value);
    }

    uint32_t getBe32U() {
        uint32_t value;
        read(&value, sizeof(value));
        return swapEndian32(value);
    }

    uint64_t getBe64U() {
        uint64_t value;
        read(&value, sizeof(value));
        return swapEndian64(value);
    }

    uint8_t peek8U() {
        uint8_t value;
        peek(&value, sizeof(value));
        return value;
    }

    uint16_t peekBe16U() {
        uint16_t value;
        peek(&value, sizeof(value));
        return swapEndian16(value);
    }

    uint32_t peekBe32U() {
        uint32_t value;
        peek(&value, sizeof(value));
        return swapEndian32(value);
    }

    uint64_t peekBe64U() {
        uint64_t value;
        peek(&value, sizeof(value));
        return swapEndian64(value);
    }

private:
    const std::vector<uint8_t>& buffer;
    bool hasSize = false;
    mutable size_t size = 0;
    mutable size_t cur = 0;
};


class WriteStream final {
public:
    WriteStream() {}

    WriteStream(const WriteStream&) = delete;
    WriteStream& operator=(const WriteStream&) = delete;

    WriteStream(WriteStream&&) = default;
    WriteStream& operator=(WriteStream&&) = default;

    template <typename T>
    size_t writeObject(const T value) {
        const uint8_t* byteBuffer = reinterpret_cast<const uint8_t*>(&value);
        buffer.insert(buffer.end(), byteBuffer, byteBuffer + sizeof(T));
        return sizeof(T);
    }

    size_t write(std::span<const uint8_t> data) {
        buffer.insert(buffer.end(), data.begin(), data.end());
        return data.size();
    }
    
    size_t write(std::initializer_list<uint8_t> data) {
        buffer.insert(buffer.end(), data.begin(), data.end());
        return data.size();
    }

    size_t put8U(uint8_t value) {
        return writeObject(value);
    }

    size_t put16U(uint16_t value) {
        return writeObject(value);
    }

    size_t put32U(uint32_t value) {
        return writeObject(value);
    }

    size_t put64U(uint64_t value) {
        return writeObject(value);
    }
    
    size_t putBe16U(uint16_t value) {
        return writeObject(swapEndian16(value));
    }

    size_t putBe32U(uint32_t value) {
        return writeObject(swapEndian32(value));
    }

    size_t putBe64U(uint64_t value) {
        return writeObject(swapEndian64(value));
    }

    const std::vector<uint8_t>& getData() const {
        return buffer;
    }

private:
    std::vector<uint8_t> buffer;

};

}

}