#pragma once
#include <vector>
#include "stream.h"
#include "ip.h"

namespace MmtTlv {
	
namespace Common {
	class Stream;
}

class MmtGeneralLocationInfo {
public:
	bool unpack(Common::ReadStream& stream);

	uint8_t locationType;
	uint16_t packetId;
	uint32_t ipv4SrcAddr;
	uint32_t ipv4DstAddr;
	struct Common::in6_addr ipv6SrcAddr;
	struct Common::in6_addr ipv6DstAddr;
	uint16_t dstPort;
	uint16_t networkId;

	uint16_t mpeg2TransportStreamId;
	uint8_t reserved;
	uint16_t mpeg2Pid;

	uint8_t urlLength;
	std::vector<uint8_t> urlByte;
};

}