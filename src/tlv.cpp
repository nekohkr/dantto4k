#include "tlv.h"
#include "stream.h"
#include "MmtTlvDemuxer.h"

bool TLVPacket::unpack(Stream& stream)
{
	if (stream.leftBytes() < 4) {
		return false;
	}

	uint8_t syncByte = stream.get8U();
	if (syncByte != 0x7F) {
		throw std::runtime_error("not valid tlv packet.");
	}

	packetType = stream.get8U();
	dataLength = stream.getBe16U();

	if (stream.leftBytes() < dataLength) {
		return false;
	}

	data.resize(dataLength);
	stream.read((char*)data.data(), dataLength);
	return true;
}

bool CompressedIPPacket::unpack(Stream& stream)
{
	uint16_t uint16 = stream.getBe16U();
	contextId = (uint16 & 0b1111111111110000) >> 4;
	sequenceNumber = uint16 & 0b0000000000001111;
	headerType = stream.get8U();

	switch (headerType) {
	case CONTEXT_ID_PARTIAL_IPV4_HEADER_AND_PARTIAL_UDP_HEADER:
		break;
	case CONTEXT_ID_IPV4_HEADER_IDENTIFIER:
		break;
	case CONTEXT_ID_PARTIAL_IPV6_HEADER_AND_PARTIAL_UDP_HEADER:
		ipv6.assign(38, 0);
		stream.read((char*)ipv6.data(), 38);

		udp.assign(4, 0);
		stream.read((char*)udp.data(), 4);
		break;
	case CONTEXT_ID_NO_COMPRESSED_HEADER:
		break;
	}
	return false;
}

bool IPv6Header::unpack(Stream& stream)
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
	return false;
}

bool TransmissionControlSignal::unpack(Stream& stream)
{
	tableId = stream.get8U();

	return false;
}
