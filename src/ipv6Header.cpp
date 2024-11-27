#include "ipv6Header.h"

namespace MmtTlv {

bool IPv6Header::unpack(Common::ReadStream& stream)
{
	uint32_t uint16 = stream.getBe16U();
	version = (uint16 & 0b1111000000000000) >> 12;
	priority = (uint16 & 0b0000111111110000) >> 4;
	flow_lbl = (uint16 & 0b0000000000001111) << 16;

	uint16 = stream.getBe16U();
	flow_lbl |= uint16;

	nexthdr = stream.get8U();
	hop_limit = stream.get8U();

	if (!isCompressed) {
		payloadLength = stream.getBe16U();
	}

	stream.read(saddr.in6_u.u6_addr8, 16);
	stream.read(daddr.in6_u.u6_addr8, 16);

	uint8_t curNexthdr = nexthdr;

	// TODO
	/*
	while (!stream.isEof()) {
		switch (curNexthdr) {

		}
	}
	*/

	return true;
}

}