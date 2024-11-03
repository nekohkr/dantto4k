#pragma once
#include <vector>
#include "stream.h"

namespace MmtTlv {

enum class ContextHeaderType : uint8_t {
	ContextIdPartialIpv4AndPartialUdp = 0x20,
	ContextIdIpv4Identifier = 0x21,
	ContextIdPartialIpv6AndPartialUdp = 0x60,
	ContextIdNoCompressedHheader = 0x61,
};

class CompressedIPPacket {
public:
	bool unpack(Common::Stream& stream);

	std::vector<uint8_t> getCompressedHeader() const {
		return compressedHeader;
	}

public:
	std::vector<uint8_t> compressedHeader;
	uint16_t contextId;
	uint8_t sequenceNumber;
	ContextHeaderType headerType;

	// Not implemented
	std::vector<uint8_t> ipv6;
	std::vector<uint8_t> udp;
	std::vector<uint8_t> ipv4;
};

}