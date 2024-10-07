#include "mmtDescriptor.h"

bool MmtDescriptor::unpack(Stream& stream, bool is16BitLength)
{
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

	return true;
}
