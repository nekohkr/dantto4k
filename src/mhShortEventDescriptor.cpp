#include "mhShortEventDescriptor.h"

namespace MmtTlv {

bool MhShortEventDescriptor::unpack(Common::ReadStream& stream) {
    try {
        if (!MmtDescriptorTemplate::unpack(stream)) {
            return false;
        }

        Common::ReadStream nstream(stream, descriptorLength);

        uint8_t  eventNameLength;
        uint16_t textLength;

        nstream.read(language, 3);
        language[3] = '\0';

        eventNameLength = nstream.get8U();
        eventName.resize(eventNameLength);
        nstream.read(eventName.data(), eventNameLength);

        textLength = nstream.getBe16U();
        text.resize(textLength);
        nstream.read(text.data(), textLength);

        stream.skip(descriptorLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

	return true;
}

}