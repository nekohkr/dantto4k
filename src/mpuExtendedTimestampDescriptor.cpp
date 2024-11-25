#include "mpuExtendedTimestampDescriptor.h"

namespace MmtTlv {

bool MpuExtendedTimestampDescriptor::unpack(Common::ReadStream& stream)
{
	try {
		if (!MmtDescriptorTemplate::unpack(stream)) {
			return false;
		}

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

		entries.reserve(50);
		while (!stream.isEof()) {
			Entry entry;
			entry.unpack(stream, ptsOffsetType, defaultPtsOffset);

			entries.push_back(entry);
		}
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

bool MpuExtendedTimestampDescriptor::Entry::unpack(Common::ReadStream& stream, uint8_t ptsOffsetType, uint16_t defaultPtsOffset)
{
	try {
		mpuSequenceNumber = stream.getBe32U();
		uint8_t uint8 = stream.get8U();
		mpuPresentationTimeLeapIndicator = (uint8 & 0b11000000) >> 6;
		reserved = uint8 & 0b00111111;
		mpuDecodingTimeOffset = stream.getBe16U();
		numOfAu = stream.get8U();

		dtsPtsOffsets.reserve(numOfAu);
		ptsOffsets.reserve(numOfAu);
		for (int i = 0; i < numOfAu; i++) {
			dtsPtsOffsets.push_back(stream.getBe16U());
			if (ptsOffsetType == 2) {
				ptsOffsets.push_back(stream.getBe16U());
			}
			else {
				ptsOffsets.push_back(defaultPtsOffset);
			}
		}
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

}