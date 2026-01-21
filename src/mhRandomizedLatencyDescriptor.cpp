#include "mhRandomizedLatencyDescriptor.h"

namespace MmtTlv {

bool MhRandomizedLatencyDescriptor::unpack(Common::ReadStream& stream) {
    try {
        if (!MmtDescriptorTemplate::unpack(stream)) {
            return false;
        }

        Common::ReadStream nstream(stream, descriptorLength);

        range = nstream.getBe16U();
        rate = nstream.get8U();

        uint8_t uint8 = nstream.get8U();
        randomizationEndTimeFlag = (uint8 & 0b10000000) >> 7;
        reserved = uint8 & 0b01111111;
        if (randomizationEndTimeFlag) {
            randomizationEndTime = nstream.getBe64U();
        }

        stream.skip(descriptorLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

}