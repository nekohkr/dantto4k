#include "tlv.h"
#include "stream.h"
#include "mmtTlvDemuxer.h"

namespace MmtTlv {
	
bool Tlv::unpack(Common::ReadStream& stream)
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
	stream.read(data.data(), dataLength);
	return true;
}

}