#include "mhParentalRatingDescriptor.h"

bool MhParentalRatingDescriptor::unpack(Stream& stream)
{
    try {
        if (!MmtDescriptor::unpack(stream)) {
            return false;
        }

        Stream nstream(stream, descriptorLength);

        while (!nstream.isEOF()) {
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

bool MhParentalRatingDescriptor::Entry::unpack(Stream& stream) {
    stream.read(countryCode, 3);
    countryCode[3] = '\0';

    rating = stream.get8U();

    return true;
}