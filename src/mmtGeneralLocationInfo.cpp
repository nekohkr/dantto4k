#include "mmtGeneralLocationInfo.h"

bool MmtGeneralLocationInfo::unpack(Stream& stream)
{
	uint16_t uint16;

	try {
		locationType = stream.get8U();
		switch (locationType) {
		case 0:
			packetId = stream.getBe16U();
			break;
		case 1:
			stream.read((char*)&ipv4SrcAddr, 4);
			stream.read((char*)&ipv4DstAddr, 4);
			dstPort = stream.getBe16U();
			packetId = stream.getBe16U();
			break;
		case 2:
			stream.read((char*)&ipv6SrcAddr, 16);
			stream.read((char*)&ipv6DstAddr, 16);
			dstPort = stream.getBe16U();
			packetId = stream.getBe16U();
			break;
		case 3:
			networkId = stream.getBe16U();
			mpeg2TransportStreamId = stream.getBe16U();

			uint16 = stream.getBe16U();
			reserved = (uint16 & 0b1110000000000000) >> 13;
			mpeg2Pid = uint16 & 0x0001111111111111;
			break;
		case 4:
			stream.read((char*)&ipv6SrcAddr, 16);
			stream.read((char*)&ipv6DstAddr, 16);
			dstPort = stream.getBe16U();

			uint16 = stream.getBe16U();
			reserved = (uint16 & 0b1110000000000000) >> 13;
			mpeg2Pid = uint16 & 0x0001111111111111;
			break;
		case 5:
			urlLength = stream.get8U();
			urlByte.resize(urlLength);
			stream.read((char*)urlByte.data(), urlLength);
			break;
		}
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}