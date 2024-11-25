#pragma once
#include <vector>
#include "ip.h"
#include "stream.h"
#include "compressedIPPacket.h"

namespace MmtTlv {

enum class TlvPacketType {
	Undefined = 0x0,
	Ipv4Packet = 0x01,
	Ipv6Packet = 0x02,
	HeaderCompressedIpPacket = 0x03,
	TransmissionControlSignalPacket = 0xFE,
	NullPacket = 0xFF
};

class Tlv {
public:
	bool unpack(Common::ReadStream& stream);

	TlvPacketType getPacketType() const { return static_cast<TlvPacketType>(packetType); }
	uint16_t getDataLength() const { return dataLength; }
	const CompressedIPPacket& getCompressedIPPacket() const { return compressedIPPacket; }
	const std::vector<uint8_t>& getData() const { return data; }

private:
	uint8_t packetType;
	uint16_t dataLength;
	CompressedIPPacket compressedIPPacket;
	std::vector<uint8_t> data;
};

}