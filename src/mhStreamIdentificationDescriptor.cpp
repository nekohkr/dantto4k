#include "mhStreamIdentificationDescriptor.h"

bool MhStreamIdentificationDescriptor::unpack(Stream& stream)
{
    try {
        if (!MmtDescriptor::unpack(stream)) {
            return false;
        }

        Stream nstream(stream, descriptorLength);

        componentTag = nstream.getBe16U();

        stream.skip(descriptorLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}
