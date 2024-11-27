#pragma once
#include "stream.h"
#include "ip.h"

namespace MmtTlv {

class IPv6Header {
public:
	IPv6Header(bool isCompressed = false) :
		isCompressed(isCompressed) {}

	bool unpack(Common::ReadStream& stream);

public:
	bool isCompressed{};
	uint8_t version{};
	uint8_t priority{};
	uint32_t flow_lbl{};
	uint8_t nexthdr{};
	uint8_t hop_limit{};


	uint16_t payloadLength{};
	uint8_t payloadType{};

	struct Common::in6_addr saddr;
	struct Common::in6_addr daddr;
};

}