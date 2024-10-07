#include "mpuExtendedTimestampDescriptor.h"


bool MpuExtendedTimestamp::unpack(Stream& stream, uint8_t ptsOffsetType, uint16_t defaultPtsOffset)
{
	mpuSequenceNumber = stream.getBe32U();
	uint8_t uint8 = stream.get8U();
	mpuPresentationTimeLeapIndicator = (uint8 & 0b11000000) >> 6;
	reserved = uint8 & 0b00111111;
	mpuDecodingTimeOffset = stream.getBe16U();
	numOfAu = stream.get8U();

	for (int i = 0; i < numOfAu; i++) {
		dtsPtsOffsets.push_back(stream.getBe16U());
		if (ptsOffsetType == 2) {
			ptsOffsets.push_back(stream.getBe16U());
		}
		else {
			ptsOffsets.push_back(defaultPtsOffset);
		}
	}

	if (numOfAu == 32 && dtsPtsOffsets.size() == 16) {
		int a = 1;
	}

	return true;
}

bool MpuExtendedTimestampDescriptor::unpack(Stream& stream)
{
	if (stream.leftBytes() < 3) {
		return false;
	}

	if (!MmtDescriptor::unpack(stream))
		return false;

	uint8_t uint8 = stream.get8U();
	reserved = (uint8 & 0b11111000) >> 3;
	ptsOffsetType = (uint8 & 0b00000110) >> 1;
	timescaleFlag = uint8 & 0b00000001;

	if (timescaleFlag) {
		timescale = stream.getBe32U();
	}
	if (ptsOffsetType == 1) {
		defaultPtsOffset = stream.getBe16U();
	}

	while (!stream.isEOF()) {
		MpuExtendedTimestamp ts;
		ts.unpack(stream, ptsOffsetType, defaultPtsOffset);

		extendedTimestamps.push_back(ts);
	}

	return true;
}