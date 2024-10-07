#pragma once
#include "stream.h"
#include "ip.h"

class MmtGeneralLocationInfo {
public:
	bool unpack(Stream& stream);

	uint8_t locationType;
	uint16_t packetId;
	uint32_t ipv4SrcAddr;
	uint32_t ipv4DstAddr;
	struct nn_in6_addr ipv6SrcAddr;
	struct nn_in6_addr ipv6DstAddr;
	uint16_t dstPort;
	uint16_t networkId;

	uint16_t mpeg2TransportStreamId;
	uint8_t reserved;
	uint16_t mpeg2Pid;

	uint8_t urlLength;
	std::vector<uint8_t> urlByte;
};