#include "mhLogoTransmissionDescriptor.h"

namespace MmtTlv {

bool MhLogoTransmissionDescriptor::unpack(Common::ReadStream& stream)
{
    try {
        if (!MmtDescriptorTemplate::unpack(stream)) {
            return false;
        }

        Common::ReadStream nstream(stream, descriptorLength);

        logoTransmissionType = nstream.get8U();
        if (logoTransmissionType == 0x01) {
            uint16_t uint16 = nstream.getBe16U();
            reservedFutureUse1 = (uint16 & 0b1111111000000000) >> 9;
            logoId = uint16 & 0b0000000111111111;

            uint16 = nstream.getBe16U();
            reservedFutureUse2 = (uint16 & 0b1111000000000000) >> 12;
            logoVersion = uint16 & 0b0000111111111111;
            downloadDataId = nstream.getBe16U();

            while (!nstream.isEof()) {
                Entry entry;
                entry.unpack(nstream);
                entries.push_back(entry);
            }
        }
        else if (logoTransmissionType == 0x02) {
            uint16_t uint16 = nstream.getBe16U();
            reservedFutureUse1 = (uint16 & 0b1111111000000000) >> 9;
            logoId = uint16 & 0b0000000111111111;
        }
        else if (logoTransmissionType == 0x03) {
            logoChar.resize(nstream.leftBytes());
            nstream.read(logoChar.data(), nstream.leftBytes());
        }

        stream.skip(descriptorLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

bool MhLogoTransmissionDescriptor::Entry::unpack(Common::ReadStream& stream)
{
    logoType = stream.get8U();
    startSectionNumber = stream.get8U();
    numOfSections = stream.get8U();
    return true;
}

}