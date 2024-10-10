#include "mmtDescriptor.h"

bool MmtDescriptor::unpack(Stream& stream, bool is16BitLength)
{
	try {
		descriptorTag = stream.getBe16U();
		if (is16BitLength) {
			descriptorLength = stream.getBe16U();
		}
		else {
			descriptorLength = stream.get8U();
		}

		if (stream.leftBytes() < descriptorLength) {
			return false;
		}
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}
