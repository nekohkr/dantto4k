#include "mhShortEventDescriptor.h"

namespace MmtTlv {

bool MhShortEventDescriptor::unpack(Common::ReadStream& stream)
{
    try {
        if (!MmtDescriptorTemplate::unpack(stream)) {
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
    }
    catch (const std::out_of_range&) {
        return false;
    }

	return true;
}

}