#include "ecm.h"

namespace MmtTlv {

bool Ecm::unpack(Common::ReadStream& stream)
{
    try {
        if (!MmtTableBase::unpack(stream)) {
            return false;
        }

        uint16_t uint16 = stream.getBe16U();
        sectionSyntaxIndicator = (uint16 & 0b1000000000000000) >> 15;
        sectionLength = uint16 & 0b0000111111111111;

        tlvStreamId = stream.getBe16U();

        uint8_t uint8 = stream.get8U();
        currentNextIndicator = uint8 & 1;
        sectionNumber = stream.get8U();
        lastSectionNumber = stream.get8U();

        ecmData.resize(stream.leftBytes() - 4);
        stream.read(ecmData.data(), stream.leftBytes() - 4);

        crc32 = stream.getBe32U();
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

}