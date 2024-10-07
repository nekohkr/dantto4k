#include "mmtGeneralLocationInfo.h"

bool MmtGeneralLocationInfo::unpack(Stream& stream)
{
	uint16_t uint16;

	if (stream.leftBytes() < 1) {
		return false;
	}

	locationType = stream.get8U();
	switch (locationType) {
	case 0:
		if (stream.leftBytes() < 2) {
			return false;
		}

		packetId = stream.getBe16U();
		break;
	case 1:
		if (stream.leftBytes() < 4 + 4 + 2 + 2) {
			return false;
		}

		stream.read((char*)&ipv4SrcAddr, 4);
		stream.read((char*)&ipv4DstAddr, 4);
		dstPort = stream.getBe16U();
		packetId = stream.getBe16U();
		break;
	case 2:
		if (stream.leftBytes() < 2 + 2) {
			return false;
		}

		stream.read((char*)&ipv6SrcAddr, 16);
		stream.read((char*)&ipv6DstAddr, 16);
		dstPort = stream.getBe16U();
		packetId = stream.getBe16U();
		break;
	case 3:
		if (stream.leftBytes() < 2 + 2 + 2) {
			return false;
		}

		networkId = stream.getBe16U();
		mpeg2TransportStreamId = stream.getBe16U();

		uint16 = stream.getBe16U();
		reserved = (uint16 & 0b1110000000000000) >> 13;
		mpeg2Pid = uint16 & 0x0001111111111111;
		break;
	case 4:
		if (stream.leftBytes() < 16 + 16 + 2 + 2) {
			return false;
		}

		stream.read((char*)&ipv6SrcAddr, 16);
		stream.read((char*)&ipv6DstAddr, 16);
		dstPort = stream.getBe16U();

		uint16 = stream.getBe16U();
		reserved = (uint16 & 0b1110000000000000) >> 13;
		mpeg2Pid = uint16 & 0x0001111111111111;
		break;
	case 5:
		if (stream.leftBytes() < 1) {
			return false;
		}

		urlLength = stream.get8U();
		urlByte.resize(urlLength);
		stream.read((char*)urlByte.data(), urlLength);
		break;
	}
	return true;
}