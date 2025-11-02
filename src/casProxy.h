#pragma once
#include <initializer_list>
#include <vector>
#include <string>
#include <optional>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <functional>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <winscard.h>

namespace casproxy {

inline std::optional<std::pair<std::string, uint16_t>> parseAddress(const std::string& addr) {
    auto pos = addr.find(':');
    if (pos == std::string::npos) {
        return std::nullopt;
    }

    std::string host = addr.substr(0, pos);
    std::string portStr = addr.substr(pos + 1);

    if (host.empty()) {
        return std::nullopt;
    }

    if (portStr.empty()) {
        return std::nullopt;
    }

    if (!std::all_of(portStr.begin(), portStr.end(), ::isdigit)) {
        return std::nullopt;
    }

    int portInt;
    try {
        portInt = std::stoi(portStr);
    }
    catch (...) {
        return std::nullopt;
    }

    if (portInt < 0 || portInt > 65535) {
        return std::nullopt;
    }

    return std::make_pair(host, static_cast<uint16_t>(portInt));
}

inline uint16_t swapEndian16(uint16_t num) {
    return (num >> 8) | (num << 8);
}

inline uint32_t swapEndian32(uint32_t num) {
    return ((num >> 24) & 0x000000FF) |
        ((num >> 8) & 0x0000FF00) |
        ((num << 8) & 0x00FF0000) |
        ((num << 24) & 0xFF000000);
}

inline uint64_t swapEndian64(uint64_t num) {
    return ((num >> 56) & 0x00000000000000FFULL) |
        ((num >> 40) & 0x000000000000FF00ULL) |
        ((num >> 24) & 0x0000000000FF0000ULL) |
        ((num >> 8) & 0x00000000FF000000ULL) |
        ((num << 8) & 0x000000FF00000000ULL) |
        ((num << 24) & 0x0000FF0000000000ULL) |
        ((num << 40) & 0x00FF000000000000ULL) |
        ((num << 56) & 0xFF00000000000000ULL);
}

inline const SCARD_IO_REQUEST* getPciByType(int32_t type) {
    switch (type) {
    case 0: return SCARD_PCI_T0;
    case 1: return SCARD_PCI_T1;
    case 2: return SCARD_PCI_RAW;
    default: return nullptr;
    }
}

enum class Opcode : uint32_t {
    SCardEstablishContextReq = 1,
    SCardEstablishContextRes,
    SCardReleaseContextReq,
    SCardReleaseContextRes,
    SCardListReadersReq,
    SCardListReadersRes,
    SCardConnectReq,
    SCardConnectRes,
    SCardDisconnectReq,
    SCardDisconnectRes,
    SCardBeginTransactionReq,
    SCardBeginTransactionRes,
    SCardEndTransactionReq,
    SCardEndTransactionRes,
    SCardTransmitReq,
    SCardTransmitRes,
    SCardGetAttribReq,
    SCardGetAttribRes,
};

class StreamWriter {
public:
    void write(const std::string& value) {
        uint32_t size = static_cast<uint32_t>(value.size());
        write(size);
        buffer.insert(buffer.end(), value.begin(), value.end());
    }

    void write(const std::vector<uint8_t>& value) {
        uint32_t size = static_cast<uint32_t>(value.size());
        write(size);
        buffer.insert(buffer.end(), value.begin(), value.end());
    }

    void write(uint32_t value) {
        uint8_t bytes[4];
        memcpy(bytes, &value, sizeof(value));
        buffer.insert(buffer.end(), bytes, bytes + 4);
    }

    void write(uint64_t value) {
        uint8_t bytes[8];
        memcpy(bytes, &value, sizeof(value));
        buffer.insert(buffer.end(), bytes, bytes + 8);
    }

    void write(bool value) {
        buffer.push_back(value ? 1 : 0);
    }

    void writeBe(const std::string& value) {
        uint32_t size = static_cast<uint32_t>(value.size());
        write(swapEndian32(size));
        buffer.insert(buffer.end(), value.begin(), value.end());
    }

    void writeBe(const std::vector<uint8_t>& value) {
        uint32_t size = static_cast<uint32_t>(value.size());
        write(swapEndian32(size));
        buffer.insert(buffer.end(), value.begin(), value.end());
    }

    void writeBe(uint32_t value) {
        uint32_t be = swapEndian32(value);
        write(be);
    }

    void writeBe(uint64_t value) {
        uint64_t be = swapEndian64(value);
        write(be);
    }

    void writeBe(bool value) {
        write(value);
    }

    std::vector<uint8_t> buffer;

};

class StreamReader {
public:
    explicit StreamReader(const std::vector<uint8_t>& data)
        : buffer_(data), offset_(0) {
    }

    bool read(std::string& value) {
        uint32_t size;
        if (!read(size)) return false;
        if (offset_ + size > buffer_.size()) return false;

        value.assign(buffer_.begin() + offset_, buffer_.begin() + offset_ + size);
        offset_ += size;
        return true;
    }

    bool read(std::vector<uint8_t>& value) {
        uint32_t size;
        if (!read(size)) return false;
        if (offset_ + size > buffer_.size()) return false;

        value.assign(buffer_.begin() + offset_, buffer_.begin() + offset_ + size);
        offset_ += size;
        return true;
    }

    bool read(uint32_t& value) {
        if (offset_ + 4 > buffer_.size()) return false;
        std::memcpy(&value, &buffer_[offset_], 4);
        offset_ += 4;
        return true;
    }

    bool read(uint64_t& value) {
        if (offset_ + 8 > buffer_.size()) return false;
        std::memcpy(&value, &buffer_[offset_], 8);
        offset_ += 8;
        return true;
    }

    bool read(bool& value) {
        if (offset_ + 1 > buffer_.size()) return false;
        value = buffer_[offset_] != 0;
        offset_ += 1;
        return true;
    }

    bool readBe(std::string& value) {
        uint32_t size;
        if (!readBe(size)) return false;
        if (offset_ + size > buffer_.size()) return false;

        value.assign(buffer_.begin() + offset_, buffer_.begin() + offset_ + size);
        offset_ += size;
        return true;
    }

    bool readBe(std::vector<uint8_t>& value) {
        uint32_t size;
        if (!readBe(size)) return false;
        if (offset_ + size > buffer_.size()) return false;

        value.assign(buffer_.begin() + offset_, buffer_.begin() + offset_ + size);
        offset_ += size;
        return true;
    }

    bool readBe(uint32_t& value) {
        if (offset_ + 4 > buffer_.size()) return false;
        memcpy(&value, &buffer_[offset_], 4);
        value = swapEndian32(value);
        offset_ += 4;
        return true;
    }

    bool readBe(uint64_t& value) {
        if (offset_ + 8 > buffer_.size()) return false;
        memcpy(&value, &buffer_[offset_], 8);
        value = swapEndian64(value);
        offset_ += 8;
        return true;
    }

    bool readBe(bool& value) {
        if (offset_ + 1 > buffer_.size()) return false;
        value = buffer_[offset_] != 0;
        offset_ += 1;
        return true;
    }

    size_t remaining() const { return buffer_.size() - offset_; }

private:
    const std::vector<uint8_t>& buffer_;
    size_t offset_;
};

class RequestBase {
public:
    virtual ~RequestBase() = default;
    virtual bool unpack(uint32_t packetId, StreamReader& reader) {
        this->packetId = packetId;
        return unpackPayload(reader);
    };
    virtual void pack(StreamWriter& writer) const {
        writer.writeBe(packetId);
        writer.writeBe(opcode);
        packPayload(writer);
    }

    uint32_t packetId;
    uint32_t opcode;

protected:
    virtual bool unpackPayload(StreamReader& reader) { return true; }
    virtual void packPayload(StreamWriter& reader) const {}

};

template<Opcode OpcodeTag>
class TypedRequest : public RequestBase {
public:
    TypedRequest() {
        opcode = static_cast<uint32_t>(OpcodeTag);
    }

};

class SCardEstablishContextRequest : public TypedRequest<Opcode::SCardEstablishContextReq> {
public:
    uint32_t dwScope{ 0 };

protected:
    virtual bool unpackPayload(StreamReader& reader) {
        if (!reader.readBe(dwScope)) {
            return false;
        }
        if (reader.remaining() > 0) {
            return false;
        }
        return true;
    }

    virtual void packPayload(StreamWriter& writer) const {
        writer.writeBe(dwScope);
    }

};

class SCardReleaseContextRequest : public TypedRequest<Opcode::SCardReleaseContextReq> {
public:
    uint64_t hContext{0};

protected:
    virtual bool unpackPayload(StreamReader& reader) {
        if (!reader.readBe(hContext)) {
            return false;
        }
        if (reader.remaining() > 0) {
            return false;
        }
        return true;
    }

    virtual void packPayload(StreamWriter& writer) const {
        writer.writeBe(hContext);
    }

};

class SCardListReadersRequest : public TypedRequest<Opcode::SCardListReadersReq> {
public:
    uint64_t hContext{0};
    bool isGroupsNull{true};
    std::string groups;
    uint32_t readersLength{0};

protected:
    virtual bool unpackPayload(StreamReader& reader) {
        if (!reader.readBe(hContext)) {
            return false;
        }
        if (!reader.readBe(isGroupsNull)) {
            return false;
        }
        if (!isGroupsNull) {
            if (!reader.readBe(groups)) {
                return false;
            }
        }
        if (!reader.readBe(readersLength)) {
            return false;
        }
        if (reader.remaining() > 0) {
            return false;
        }
        return true;
    }

    virtual void packPayload(StreamWriter& writer) const {
        writer.writeBe(hContext);
        writer.writeBe(isGroupsNull);
        if (!isGroupsNull) {
            writer.writeBe(groups);
        }
        writer.writeBe(readersLength);
    }

};

class SCardConnectRequest : public TypedRequest<Opcode::SCardConnectReq> {
public:
    uint64_t hContext{0};
    std::string szReader;
    uint32_t dwShareMode{0};
    uint32_t dwPreferredProtocols{0};

protected:
    virtual bool unpackPayload(StreamReader& reader) {
        if (!reader.readBe(hContext)) {
            return false;
        }
        if (!reader.readBe(szReader)) {
            return false;
        }
        if (!reader.readBe(dwShareMode)) {
            return false;
        }
        if (!reader.readBe(dwPreferredProtocols)) {
            return false;
        }
        if (reader.remaining() > 0) {
            return false;
        }
        return true;
    }

    virtual void packPayload(StreamWriter& writer) const {
        writer.writeBe(hContext);
        writer.writeBe(szReader);
        writer.writeBe(dwShareMode);
        writer.writeBe(dwPreferredProtocols);
    }

};

class SCardDisconnectRequest : public TypedRequest<Opcode::SCardDisconnectReq> {
public:
    uint64_t hCard{0};
    uint32_t dwDisposition{0};

protected:
    virtual bool unpackPayload(StreamReader& reader) {
        if (!reader.readBe(hCard)) {
            return false;
        }
        if (!reader.readBe(dwDisposition)) {
            return false;
        }
        if (reader.remaining() > 0) {
            return false;
        }
        return true;
    }

    virtual void packPayload(StreamWriter& writer) const {
        writer.writeBe(hCard);
        writer.writeBe(dwDisposition);
    }

};

class SCardBeginTransactionRequest : public TypedRequest<Opcode::SCardBeginTransactionReq> {
public:
    uint64_t hCard{0};

protected:
    virtual bool unpackPayload(StreamReader& reader) {
        if (!reader.readBe(hCard)) {
            return false;
        }
        if (reader.remaining() > 0) {
            return false;
        }
        return true;
    }

    virtual void packPayload(StreamWriter& writer) const {
        writer.writeBe(hCard);
    }

};

class SCardEndTransactionRequest : public TypedRequest<Opcode::SCardEndTransactionReq> {
public:
    uint64_t hCard{0};
    uint32_t dwDisposition{0};

protected:
    virtual bool unpackPayload(StreamReader& reader) {
        if (!reader.readBe(hCard)) {
            return false;
        }
        if (!reader.readBe(dwDisposition)) {
            return false;
        }
        if (reader.remaining() > 0) {
            return false;
        }
        return true;
    }

    virtual void packPayload(StreamWriter& writer) const {
        writer.writeBe(hCard);
        writer.writeBe(dwDisposition);
    }

};

class SCardTransmitRequest : public TypedRequest<Opcode::SCardTransmitReq> {
public:
    uint64_t hCard{0};
    uint32_t sendPci{0};
    std::vector<uint8_t> sendBuffer;
    bool isRecvPciNull{true};
    uint32_t recvPciProtocol{0};
    uint32_t recvPciLength{0};
    uint32_t recvLength{0};

protected:
    virtual bool unpackPayload(StreamReader& reader) {
        if (!reader.readBe(hCard)) {
            return false;
        }
        if (!reader.readBe(sendPci)) {
            return false;
        }
        if (!reader.readBe(sendBuffer)) {
            return false;
        }
        if (!reader.readBe(isRecvPciNull)) {
            return false;
        }
        if (!isRecvPciNull) {
            if (!reader.readBe(recvPciProtocol)) {
                return false;
            }
            if (!reader.readBe(recvPciLength)) {
                return false;
            }
        }
        if (!reader.readBe(recvLength)) {
            return false;
        }

        return true;
    }

    virtual void packPayload(StreamWriter& writer) const {
        writer.writeBe(hCard);
        writer.writeBe(sendPci);
        writer.writeBe(sendBuffer);
        writer.writeBe(isRecvPciNull);
        if (!isRecvPciNull) {
            writer.writeBe(isRecvPciNull);
            writer.writeBe(recvPciLength);
        }
        writer.writeBe(recvLength);
    }

};

class SCardGetAttribRequest : public TypedRequest<Opcode::SCardGetAttribReq> {
public:
    uint64_t hCard{0};
    uint32_t dwAttrId{0};
    uint32_t attrLength{0};

protected:
    virtual bool unpackPayload(StreamReader& reader) {
        if (!reader.readBe(hCard)) {
            return false;
        }
        if (!reader.readBe(dwAttrId)) {
            return false;
        }
        if (!reader.readBe(attrLength)) {
            return false;
        }

        return true;
    }

    virtual void packPayload(StreamWriter& writer) const {
        writer.writeBe(hCard);
        writer.writeBe(dwAttrId);
        writer.writeBe(attrLength);
    }

};


class ResponseBase {
public:
    virtual ~ResponseBase() = default;

    virtual bool unpack(uint32_t packetId, uint32_t resultCode, StreamReader& reader) {
        this->packetId = packetId;
        this->resultCode = resultCode;
        return unpackPayload(reader);
    };

    virtual void pack(StreamWriter& writer) const {
        writer.writeBe(packetId);
        writer.writeBe(resultCode);
        writer.writeBe(opcode);
        packPayload(writer);
    }

    virtual bool unpackPayload(StreamReader& reader) = 0;
    virtual void packPayload(StreamWriter& writer) const = 0;

public:
    uint32_t packetId{0};
    uint32_t resultCode{0};
    uint32_t opcode{0};

};

template<Opcode OpcodeTag>
class TypedResponse : public ResponseBase {
public:
    TypedResponse() {
        opcode = static_cast<uint32_t>(OpcodeTag);
    }

};

class SCardEstablishContextResponse : public TypedResponse<Opcode::SCardEstablishContextRes> {
public:
    uint32_t apiReturn{0};
    uint64_t hContext{0};

protected:
    virtual bool unpackPayload(StreamReader& reader) {
        if (!reader.readBe(apiReturn)) {
            return false;
        }
        if (!reader.readBe(hContext)) {
            return false;
        }

        return true;
    }

    virtual void packPayload(StreamWriter& writer) const {
        writer.writeBe(apiReturn);
        writer.writeBe(hContext);
    }

};

class SCardReleaseContextResponse : public TypedResponse<Opcode::SCardReleaseContextRes> {
public:
    uint32_t apiReturn{0};

protected:
    virtual bool unpackPayload(StreamReader& reader) {
        if (!reader.readBe(resultCode)) {
            return false;
        }
        if (!reader.readBe(apiReturn)) {
            return false;
        }

        return true;
    }

    virtual void packPayload(StreamWriter& writer) const {
        writer.writeBe(resultCode);
        writer.writeBe(apiReturn);
    }

};

class SCardListReadersResponse : public TypedResponse<Opcode::SCardListReadersRes> {
public:
    uint32_t apiReturn{0};
    std::vector<uint8_t> readers;
    uint32_t readersLength{0};

protected:
    virtual bool unpackPayload(StreamReader& reader) {
        if (!reader.readBe(apiReturn)) {
            return false;
        }
        if (!reader.readBe(readers)) {
            return false;
        }
        if (!reader.readBe(readersLength)) {
            return false;
        }

        return true;
    }

    virtual void packPayload(StreamWriter& writer) const {
        writer.writeBe(apiReturn);
        writer.writeBe(readers);
        writer.writeBe(readersLength);
    }

};

class SCardConnectResponse : public TypedResponse<Opcode::SCardConnectRes> {
public:
    uint32_t apiReturn{0};
    uint64_t hCard{0};
    uint32_t dwActiveProtocol{0};

protected:
    virtual bool unpackPayload(StreamReader& reader) {
        if (!reader.readBe(apiReturn)) {
            return false;
        }
        if (!reader.readBe(hCard)) {
            return false;
        }
        if (!reader.readBe(dwActiveProtocol)) {
            return false;
        }

        return true;
    }

    virtual void packPayload(StreamWriter& writer) const {
        writer.writeBe(apiReturn);
        writer.writeBe(hCard);
        writer.writeBe(dwActiveProtocol);
    }

};

class SCardDisconnectResponse : public TypedResponse<Opcode::SCardDisconnectRes> {
public:
    uint32_t apiReturn{0};

protected:
    virtual bool unpackPayload(StreamReader& reader) {
        if (!reader.readBe(apiReturn)) {
            return false;
        }

        return true;
    }

    virtual void packPayload(StreamWriter& writer) const {
        writer.writeBe(apiReturn);
    }

};

class SCardBeginTransactionResponse : public TypedResponse<Opcode::SCardBeginTransactionRes> {
public:
    uint32_t apiReturn{0};

protected:
    virtual bool unpackPayload(StreamReader& reader) {
        if (!reader.readBe(apiReturn)) {
            return false;
        }

        return true;
    }

    virtual void packPayload(StreamWriter& writer) const {
        writer.writeBe(apiReturn);
    }

};

class SCardEndTransactionResponse : public TypedResponse<Opcode::SCardEndTransactionRes> {
public:
    uint32_t apiReturn{0};

protected:
    virtual bool unpackPayload(StreamReader& reader) {
        if (!reader.readBe(apiReturn)) {
            return false;
        }

        return true;
    }

    virtual void packPayload(StreamWriter& writer) const {
        writer.writeBe(apiReturn);
    }

};

class SCardTransmitResponse : public TypedResponse<Opcode::SCardTransmitRes> {
public:
    uint32_t apiReturn{0};
    std::vector<uint8_t> recvBuffer;
    uint32_t recvLength{0};
    bool isRecvPciNull{true};
    uint32_t recvPciProtocol{0};
    uint32_t recvPciLength{0};

protected:
    virtual bool unpackPayload(StreamReader& reader) {
        if (!reader.readBe(apiReturn)) {
            return false;
        }
        if (!reader.readBe(recvBuffer)) {
            return false;
        }
        if (!reader.readBe(recvLength)) {
            return false;
        }
        if (!reader.readBe(isRecvPciNull)) {
            return false;
        }
        if (!isRecvPciNull) {
            if (!reader.readBe(recvPciProtocol)) {
                return false;
            }
            if (!reader.readBe(recvPciLength)) {
                return false;
            }
        }

        return true;
    }

    virtual void packPayload(StreamWriter& writer) const {
        writer.writeBe(apiReturn);
        writer.writeBe(recvBuffer);
        writer.writeBe(recvLength);
        writer.writeBe(isRecvPciNull);
        if (!isRecvPciNull) {
            writer.writeBe(recvPciProtocol);
            writer.writeBe(recvPciLength);
        }
    }

};

class SCardGetAttribResponse : public TypedResponse<Opcode::SCardGetAttribRes> {
public:
    uint32_t apiReturn{0};
    std::vector<uint8_t> attrBuffer;
    uint32_t attrLength{0};

protected:
    virtual bool unpackPayload(StreamReader& reader) {
        if (!reader.readBe(apiReturn)) {
            return false;
        }
        if (!reader.readBe(attrBuffer)) {
            return false;
        }
        if (!reader.readBe(attrLength)) {
            return false;
        }

        return true;
    }

    virtual void packPayload(StreamWriter& writer) const {
        writer.writeBe(apiReturn);
        writer.writeBe(attrBuffer);
        writer.writeBe(attrLength);
    }

};

class ResponseFactory {
public:
    static std::shared_ptr<ResponseBase> create(Opcode opcode) {
        auto it = mapResponse.find(opcode);
        if (it != mapResponse.end()) {
            return it->second();
        }
        return nullptr;
    }

private:
    template<typename T>
    std::shared_ptr<ResponseBase> createResponse() {
        return std::make_shared<T>();
    }

    static inline const std::unordered_map<Opcode, std::function<std::shared_ptr<ResponseBase>()>> mapResponse = {
        { Opcode::SCardEstablishContextRes,		[]{ return std::make_shared<SCardEstablishContextResponse>(); } },
        { Opcode::SCardReleaseContextRes,		[]{ return std::make_shared<SCardReleaseContextResponse>(); } },
        { Opcode::SCardListReadersRes,			[]{ return std::make_shared<SCardListReadersResponse>(); } },
        { Opcode::SCardConnectRes,			    []{ return std::make_shared<SCardConnectResponse>(); } },
        { Opcode::SCardDisconnectRes,			[]{ return std::make_shared<SCardDisconnectResponse>(); } },
        { Opcode::SCardBeginTransactionRes,		[]{ return std::make_shared<SCardBeginTransactionResponse>(); } },
        { Opcode::SCardEndTransactionRes,		[]{ return std::make_shared<SCardEndTransactionResponse>(); } },
        { Opcode::SCardTransmitRes,			    []{ return std::make_shared<SCardTransmitResponse>(); } },
        { Opcode::SCardGetAttribRes,			[]{ return std::make_shared<SCardGetAttribResponse>(); } },
    };
};

}