#include "mhContentDescriptor.h"


bool MhContentDescriptor::unpack(Stream& stream)
{
	try {
		if (!MmtDescriptor::unpack(stream)) {
			return false;
		}

		Stream nstream(stream, descriptorLength);

		while (!nstream.isEOF()) {
			Entry entry;
			if (!entry.unpack(nstream)) {
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

bool MhContentDescriptor::Entry::unpack(Stream& stream)
{
	try {
		uint8_t uint8 = stream.get8U();
		contentNibbleLevel1 = (uint8 & 0b11110000) >> 4;
		contentNibbleLevel2 = uint8 & 0b1111;

		uint8 = stream.get8U();
		userNibble1 = (uint8 & 0b11110000) >> 4;
		userNibble2 = uint8 & 0b1111;
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}