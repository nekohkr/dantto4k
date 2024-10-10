#include "mhTot.h"

bool MhTot::unpack(Stream& stream)
{
	try {
		if (!MmtTable::unpack(stream)) {
			return false;
		}

		uint16_t uint16 = stream.getBe16U();
		sectionSyntaxIndicator = (uint16 & 0b1000000000000000) >> 15;
		sectionLength = uint16 & 0b0000111111111111;

		uint64_t uint64 = stream.getBe64U();
		jstTime = (uint64 & 0xFFFFFFFFFF000000) >> 24;
	}
	catch (const std::out_of_range&) {
		return false;
	}
	return true;
}
