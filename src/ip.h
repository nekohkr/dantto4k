#pragma once
#include <cstdint>

namespace MmtTlv {

namespace Common {

struct in6_addr
{
	union
	{
		uint8_t                u6_addr8[16];
		uint16_t                u6_addr16[8];
		uint32_t                u6_addr32[4];
	} in6_u;
};

}

}