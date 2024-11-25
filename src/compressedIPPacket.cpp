#include "compressedIPPacket.h"
#include <vector>
#include "stream.h"

namespace MmtTlv {

bool CompressedIPPacket::unpack(Common::ReadStream& stream)
{
	uint16_t uint16 = stream.getBe16U();
	contextId = (uint16 & 0b1111111111110000) >> 4;
	sequenceNumber = uint16 & 0b0000000000001111;
	headerType = static_cast<ContextHeaderType>(stream.get8U());

	switch (headerType) {
	case ContextHeaderType::ContextIdPartialIpv4AndPartialUdp:
		break;
	case ContextHeaderType::ContextIdIpv4Identifier:
		break;
	case ContextHeaderType::ContextIdPartialIpv6AndPartialUdp:
		ipv6.assign(38, 0);
		stream.read(ipv6.data(), 38);

		udp.assign(4, 0);
		stream.read(udp.data(), 4);
		break;
	case ContextHeaderType::ContextIdNoCompressedHheader:
		break;
	}
	return true;
}

}