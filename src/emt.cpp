#include "emt.h"

namespace MmtTlv {

bool Emt::unpack(Common::ReadStream& stream) {
    try {
        if (!MmtTableBase::unpack(stream)) {
            return false;
        }

        uint16_t uint16 = stream.getBe16U();
        sectionSyntaxIndicator = (uint16 & 0b1000000000000000) >> 15;
        sectionLength = uint16 & 0b0000111111111111;

        uint16 = stream.getBe16U();
        dataEventId = ((uint16 & 0b1111000000000000) >> 12);
        eventMsgGroupId = ((uint16 & 0b1111000000000000) >> 12);

        uint8_t uint8 = stream.get8U();
        currentNextIndicator = uint8 & 1;
        sectionNumber = stream.get8U();
        lastSectionNumber = stream.get8U();

        size_t descriptorsLength = stream.leftBytes() - 4;
        Common::ReadStream nstream(stream, descriptorsLength);
        if (!descriptors.unpack(nstream)) {
            return false;
        }

        stream.skip(descriptorsLength);

        crc32 = stream.getBe32U();
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

}
