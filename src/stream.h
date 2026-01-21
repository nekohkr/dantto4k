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
    explicit ReadStream(const std::vector<uint8_t>& data, size_t size);
    explicit ReadStream(ReadStream& stream, size_t size);
    explicit ReadStream(ReadStream& stream);

    ReadStream(const ReadStream&) = delete;
    ReadStream& operator=(const ReadStream&) = delete;

    ReadStream(ReadStream&&) = default;
    ReadStream& operator=(ReadStream&&) = default;

    bool isEof() const { return size == pos; }
    size_t leftBytes() const { return size - pos; }
    size_t getPos() const { return pos; }

    void seek(size_t pos) {
        if (size < pos) {
            throw std::out_of_range("Access out of bounds");
        }
        this->pos = pos;
    }

    void skip(uint64_t pos) {
        if (size < this->pos + pos) {
            throw std::out_of_range("Access out of bounds");
        }
        this->pos += pos;
    }

    size_t read(void* dst, size_t size) {
        size_t readBytes = peek(dst, size);
        pos += readBytes;
        return readBytes;
    }

    size_t read(std::span<uint8_t> data) {
        size_t readBytes = peek(data);
        pos += readBytes;
        return readBytes;
    }

    size_t peek(void* dst, size_t size) {
        if (this->size < pos + size) {
            throw std::out_of_range("Access out of bounds");
        }

        memcpy(dst, buffer.data() + pos, size);
        return size;
    }

    size_t peek(std::span<uint8_t> data) {
        if (size < pos + data.size()) {
            throw std::out_of_range("Access out of bounds");
        }

        std::copy(buffer.begin() + pos, buffer.begin() + pos + data.size(), data.begin());
        return size;
    }

    uint8_t get8U() {
        auto value = peek8U();
        skip(sizeof(value));
        return value;
    }

    uint16_t getBe16U() {
        auto value = peekBe16U();
        skip(sizeof(value));
        return value;
    }

    uint32_t getBe32U() {
        auto value = peekBe32U();
        skip(sizeof(value));
        return value;
    }

    uint64_t getBe64U() {
        auto value = peekBe64U();
        skip(sizeof(value));
        return value;
    }

    uint8_t peek8U() {
        return peekObject<uint8_t>();
    }

    uint16_t peekBe16U() {
        return swapEndian16(peekObject<uint16_t>());
    }

    uint32_t peekBe32U() {
        return swapEndian32(peekObject<uint32_t>());
    }

    uint64_t peekBe64U() {
        return swapEndian64(peekObject<uint64_t>());
    }

private:
    template<typename T>
    T peekObject() {
        if (size < pos + sizeof(T)) {
            throw std::out_of_range("Access out of bounds");
        }
        return *reinterpret_cast<const T*>(buffer.data() + pos);
    }

    const std::vector<uint8_t>& buffer;
    bool hasSize = false;
    mutable size_t size = 0;
    mutable size_t pos = 0;
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

    size_t write(std::string data) {
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
