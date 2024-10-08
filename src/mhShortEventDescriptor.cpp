#include "mhShortEventDescriptor.h"

MhShortEventDescriptor::~MhShortEventDescriptor()
{
}

bool MhShortEventDescriptor::unpack(Stream& stream)
{
    if (stream.leftBytes() < 4) {
        return false;
    }

    if (!MmtDescriptor::unpack(stream, true)) {
        return false;
    }

    uint8_t  eventNameLength;
    uint16_t textLength;

    stream.read(language, 3);
    language[3] = '\0';

    eventNameLength = stream.get8U();
    eventName.resize(eventNameLength);
    stream.read(eventName.data(), eventNameLength);

    textLength = stream.getBe16U();
    text.resize(textLength);
    stream.read(text.data(), textLength);

	return true;
}
