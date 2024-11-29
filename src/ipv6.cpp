#include "ipv6.h"

namespace MmtTlv {

bool IPv6Header::unpack(Common::ReadStream& stream)
{
	uint32_t uint16 = stream.getBe16U();
	version = (uint16 & 0b1111000000000000) >> 12;
	priority = (uint16 & 0b0000111111110000) >> 4;
	flow_lbl = (uint16 & 0b0000000000001111) << 16;

	uint16 = stream.getBe16U();
	flow_lbl |= uint16;

	if (!isCompressed) {
		payloadLength = stream.getBe16U();
	}

	nexthdr = stream.get8U();
	hop_limit = stream.get8U();

	stream.read(saddr.in6_u.u6_addr8, 16);
	stream.read(daddr.in6_u.u6_addr8, 16);

	return true;
}

bool IPv6ExtensionHeader::unpack(Common::ReadStream& stream, bool headerLengthOnly)
{
	if (!headerLengthOnly) {
		next_header = stream.get8U();
	}
	header_length = stream.get8U();

	return true;
}

bool UDPHeader::unpack(Common::ReadStream& stream, bool headerLengthOnly)
{
	source_port = stream.getBe16U();
	destination_port = stream.getBe16U();
	length = stream.getBe16U();
	checksum = stream.getBe16U();

	return true;
}

}