#include "mhContentDescriptor.h"

bool MhContentItem::unpack(Stream& stream)
{
	if (stream.leftBytes() < 2) {
		return false;
	}
	
	uint8_t uint8 = stream.get8U();
	content_nibble_level_1 = (uint8 & 0b11110000) >> 4;
	content_nibble_level_1 = uint8 & 0b1111;

	uint8 = stream.get8U();
	user_nibble1 = (uint8 & 0b11110000) >> 4;
	user_nibble2 = uint8 & 0b1111;
	return true;
}

bool MhContentDescriptor::unpack(Stream& stream)
{
	if (stream.leftBytes() < 3) {
		return false;
	}

	if (!MmtDescriptor::unpack(stream)) {
		return false;
	}

	Stream nstream(stream, descriptorLength);

	while (!nstream.isEOF()) {
		MhContentItem item;
		if (!item.unpack(nstream)) {
			return false;
		}
		items.push_back(item);
	}

	stream.skip(descriptorLength);
	return true;
}
