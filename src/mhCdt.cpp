#include "mhCdt.h"

namespace MmtTlv {

bool MhCdt::unpack(Common::ReadStream& stream)
{
    try {
        if (!MmtTableBase::unpack(stream)) {
            return false;
        }

        uint16_t uint16 = stream.getBe16U();
        sectionSyntaxIndicator = (uint16 & 0b1000000000000000) >> 15;
        sectionLength = uint16 & 0b0000111111111111;

        downloadDataId = stream.getBe16U();

        uint8_t uint8 = stream.get8U();
        versionNumber = (uint8 & 0b00111110) >> 1;
        currentNextIndicator = uint8 & 1;

        sectionNumber = stream.get8U();
        lastSectionNumber = stream.get8U();
        originalNetworkId = stream.getBe16U();
        dataType = stream.get8U();

        uint16 = stream.getBe16U();
        reservedFutureUse = (uint16 & 0b1111000000000000) >> 12;
        descriptorsLoopLength = uint16 & 0b0000111111111111;

        if (stream.leftBytes() < descriptorsLoopLength) {
            return false;
        }

        Common::ReadStream nstream(stream, descriptorsLoopLength);
        descriptors.unpack(nstream);
        stream.skip(descriptorsLoopLength);

        dataModuleByte.resize(stream.leftBytes() - 4);
        stream.read(dataModuleByte.data(), stream.leftBytes() - 4);

        if (stream.leftBytes() < 4) {
            return false;
        }

        crc32 = stream.getBe32U();
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

}