#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <vector>
#include "swap.h"

namespace MmtTlv {
    
namespace Common {

class StreamBase {
public:
    explicit StreamBase() = default;
    virtual ~StreamBase() = default;

    virtual bool isEof() const { return true; }

    virtual size_t leftBytes() { return 0; }

    virtual void skip(uint64_t pos) { }

    virtual size_t read(void* buffer, size_t size) { return 0; }

    virtual size_t peek(void* buffer, size_t size) { return 0; }

    uint8_t get8U() {
        uint8_t buffer;
        read((char*)&buffer, sizeof(buffer));
        return buffer;
    }

    uint16_t getBe16U() {
        uint16_t buffer;
        read((char*)&buffer, sizeof(buffer));
        return swapEndian16(buffer);
    }

    uint32_t getBe32U() {
        uint32_t buffer;
        read((char*)&buffer, sizeof(buffer));
        return swapEndian32(buffer);
    }

    uint64_t getBe64U() {
        uint64_t buffer;
        read((char*)&buffer, sizeof(buffer));
        return swapEndian64(buffer);
    }

    uint8_t peek8U() {
        uint8_t buffer;
        peek((char*)&buffer, sizeof(buffer));
        return buffer;
    }

    uint16_t peekBe16U() {
        uint16_t buffer;
        peek((char*)&buffer, sizeof(buffer));
        return swapEndian16(buffer);
    }

    uint32_t peekBe32U() {
        uint32_t buffer;
        peek((char*)&buffer, sizeof(buffer));
        return swapEndian32(buffer);
    }

    uint64_t peekBe64U() {
        uint64_t buffer;
        peek((char*)&buffer, sizeof(buffer));
        return swapEndian64(buffer);
    }

};

class Stream : public StreamBase {
public:
    explicit Stream(const std::vector<uint8_t>& data);
    explicit Stream(const std::vector<uint8_t>& data, uint32_t size);
    explicit Stream(Stream& stream, uint32_t size);
    explicit Stream(Stream& stream);
    virtual ~Stream() = default;

    Stream(const Stream&) = delete;
    Stream& operator=(const Stream&) = delete;

    Stream(Stream&&) = default;
    Stream& operator=(Stream&&) = default;

    virtual bool isEof() const {
        return size == cur;
    }

    virtual size_t leftBytes() {
        return size - cur;
    }

    virtual void skip(uint64_t pos) {
        if (size < cur + pos) {
            throw std::out_of_range("");
        }
        cur += pos;
    }

    virtual size_t read(void* buffer, size_t size) {
        size_t ret = peek(buffer, size);
        cur += size;
        return size;
    }

    virtual size_t peek(void* buffer, size_t size) {
        if (this->size < cur + size) {
            throw std::out_of_range("");
        }

        memcpy(buffer, data.data() + cur, size);
        return size;
    }

    uint8_t get8U() {
        uint8_t buffer;
        read((char*)&buffer, sizeof(buffer));
        return buffer;
    }

    uint16_t getBe16U() {
        uint16_t buffer;
        read((char*)&buffer, sizeof(buffer));
        return swapEndian16(buffer);
    }

    uint32_t getBe32U() {
        uint32_t buffer;
        read((char*)&buffer, sizeof(buffer));
        return swapEndian32(buffer);
    }

    uint64_t getBe64U() {
        uint64_t buffer;
        read((char*)&buffer, sizeof(buffer));
        return swapEndian64(buffer);
    }

    uint8_t peek8U() {
        uint8_t buffer;
        peek((char*)&buffer, sizeof(buffer));
        return buffer;
    }

    uint16_t peekBe16U() {
        uint16_t buffer;
        peek((char*)&buffer, sizeof(buffer));
        return swapEndian16(buffer);
    }

    uint32_t peekBe32U() {
        uint32_t buffer;
        peek((char*)&buffer, sizeof(buffer));
        return swapEndian32(buffer);
    }

    uint64_t peekBe64U() {
        uint64_t buffer;
        peek((char*)&buffer, sizeof(buffer));
        return swapEndian64(buffer);
    }

    uint64_t cur = 0;

protected:
    const std::vector<uint8_t>& data;
    bool hasSize = false;
    uint64_t size = 0;
};

class FileStream : public StreamBase {
public:
    explicit FileStream(const std::string& filename) {
        fs.open(filename, std::ios::in | std::ios::binary);
        if (!fs.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }
    }

    FileStream(const FileStream&) = delete;
    FileStream& operator=(const FileStream&) = delete;

    FileStream(FileStream&&) = default;
    FileStream& operator=(FileStream&&) = default;

    ~FileStream() override {
        if (fs.is_open()) {
            fs.close();
        }
    }

    virtual size_t leftBytes() {
        std::streampos current = fs.tellg();
        fs.seekg(0, std::ios::end);
        std::streampos end = fs.tellg();
        fs.seekg(current, std::ios::beg);

        return end - current;
    }

    virtual bool isEof() const {
        return fs.eof();
    }

    void skip(uint64_t pos) override {
        fs.seekg(pos, std::ios::cur);

        if (fs.fail()) {
            throw std::out_of_range("Failed to seek in file.");
        }
    }

    size_t read(void* buffer, size_t size) override {
        fs.read((char*)buffer, size);
        if (fs.fail() && !fs.eof()) {
            throw std::out_of_range("Failed to read from file.");
        }
        return fs.gcount();
    }

    size_t peek(void* buffer, size_t size) override {
        std::streampos currentPos = fs.tellg();
        fs.read((char*)buffer, size);

        if (fs.fail() && !fs.eof()) {
            throw std::out_of_range("Failed to read from file.");
        }
        fs.seekg(currentPos);
        return fs.gcount();
    }

private:
    std::ifstream fs;
};

}

}