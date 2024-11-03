#include "mhParentalRatingDescriptor.h"

namespace MmtTlv {

bool MhParentalRatingDescriptor::unpack(Common::Stream& stream)
{
    try {
        if (!MmtDescriptorTemplate::unpack(stream)) {
            return false;
        }

        Common::Stream nstream(stream, descriptorLength);

        while (!nstream.isEof()) {
            Entry entry;
            entry.unpack(nstream);
            entries.push_back(entry);
        }

        stream.skip(descriptorLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

bool MhParentalRatingDescriptor::Entry::unpack(Common::Stream& stream) {
    stream.read(countryCode, 3);
    countryCode[3] = '\0';

    rating = stream.get8U();

    return true;
}

}