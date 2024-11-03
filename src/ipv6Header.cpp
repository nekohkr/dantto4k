#include "ipv6Header.h"

namespace MmtTlv {

bool IPv6Header::unpack(Common::Stream& stream)
{
	uint32_t uint32 = stream.getBe16U();
	version = (uint32 & 0b11110000000000000000000000000000) >> 28;
	priority = (uint32 & 0b00001111111100000000000000000000) >> 19;
	flow_lbl = (uint32 & 0b00000000000011111111111111111111) >> 19;
	nexthdr = stream.get8U();
	hop_limit = stream.get8U();

	saddr.in6_u.u6_addr32[0] = stream.getBe32U();
	saddr.in6_u.u6_addr32[1] = stream.getBe32U();
	saddr.in6_u.u6_addr32[2] = stream.getBe32U();
	saddr.in6_u.u6_addr32[3] = stream.getBe32U();

	daddr.in6_u.u6_addr32[0] = stream.getBe32U();
	daddr.in6_u.u6_addr32[1] = stream.getBe32U();
	daddr.in6_u.u6_addr32[2] = stream.getBe32U();
	daddr.in6_u.u6_addr32[3] = stream.getBe32U();
	return true;
}

}