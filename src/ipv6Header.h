#pragma once
#include "stream.h"
#include "ip.h"

namespace MmtTlv {

class IPv6Header {
public:
	bool unpack(Common::ReadStream& stream);

private:
	uint8_t version;
	uint8_t priority;
	uint32_t flow_lbl;
	uint8_t nexthdr;
	uint8_t hop_limit;

	struct Common::in6_addr saddr;
	struct Common::in6_addr daddr;
};

}