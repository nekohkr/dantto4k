#include "mpuExtendedTimestampDescriptor.h"

namespace MmtTlv {

bool MpuExtendedTimestampDescriptor::unpack(Common::ReadStream& stream) {
	try {
		if (!MmtDescriptorTemplate::unpack(stream)) {
			return false;
		}

		Common::ReadStream nstream(stream, descriptorLength);

		uint8_t uint8 = nstream.get8U();
		reserved = (uint8 & 0b11111000) >> 3;
		ptsOffsetType = (uint8 & 0b00000110) >> 1;
		timescaleFlag = uint8 & 0b00000001;

		if (timescaleFlag) {
			timescale = nstream.getBe32U();
		}
		if (ptsOffsetType == 1) {
			defaultPtsOffset = nstream.getBe16U();
		}

		entries.reserve(15);
		while (!nstream.isEof()) {
			Entry entry;
			if (!entry.unpack(nstream, ptsOffsetType, defaultPtsOffset)) {
				return false;
			}

			entries.push_back(entry);
		}

		stream.skip(descriptorLength);
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

bool MpuExtendedTimestampDescriptor::Entry::unpack(Common::ReadStream& stream, uint8_t ptsOffsetType, uint16_t defaultPtsOffset) {
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
			if (ptsOffsetType == 1) {
				ptsOffsets.push_back(defaultPtsOffset);
			}
			else if (ptsOffsetType == 2) {
				ptsOffsets.push_back(stream.getBe16U());
			}
			else {
				ptsOffsets.push_back(0);
			}
		}
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

}