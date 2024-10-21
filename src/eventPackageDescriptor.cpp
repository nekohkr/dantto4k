#include "eventPackageDescriptor.h"

bool EventPackageDescriptor::unpack(Stream& stream)
{
    try {
        if (!MmtDescriptor::unpack(stream)) {
            return false;
        }

        Stream nstream(stream, descriptorLength);

        mmtPackageIdLength = nstream.get8U();

        mmtPackageIdByte.resize(mmtPackageIdLength);
        nstream.read(mmtPackageIdByte.data(), mmtPackageIdLength);

        stream.skip(descriptorLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}
