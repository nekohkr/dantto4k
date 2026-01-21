#include "mhCacheControlInfoDescriptor.h"

namespace MmtTlv {

bool MhCacheControlInfoDescriptor::unpack(Common::ReadStream & stream) {
    try {
        if (!MmtDescriptorTemplate::unpack(stream)) {
            return false;
        }

        Common::ReadStream nstream(stream, descriptorLength);

        applicationSize = nstream.getBe16U();
        cachePriority = nstream.get8U();

        uint8_t uint8 = nstream.get8U();
        packageFlag = (uint8 & 0b10000000) >> 7;
        applicationVersion = uint8 & 0b01111111;
        expireDate = nstream.getBe16U();

        stream.skip(descriptorLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return false;
}

}