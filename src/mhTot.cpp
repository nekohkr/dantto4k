#include "mhTot.h"

bool MhTot::unpack(Stream& stream)
{
	if (!MmtTable::unpack(stream)) {
		return false;
	}

	if (stream.leftBytes() < 2 + 8) {
		return false;
	}

	uint16_t uint16 = stream.getBe16U();
	sectionSyntaxIndicator = (uint16 & 0b1000000000000000) >> 15;
	sectionLength = uint16 & 0b0000111111111111;

	uint64_t uint64 = stream.getBe64U();
	jstTime = (uint64 & 0xFFFFFFFFFF000000) >> 24;
	return true;
}
