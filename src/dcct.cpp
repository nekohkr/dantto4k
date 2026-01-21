#include "dcct.h"

namespace MmtTlv {

bool Dcct::PU::unpack(Common::ReadStream& stream) {
    try {
        puTag = stream.getBe16U();
        puSize = stream.get8U();
        uint8_t numOfNodeTags = stream.get8U();
        for (int i = 0; i < numOfNodeTags; i++) {
            nodeTags.push_back(stream.getBe16U());
        }

        uint16_t descriptorLength = stream.getBe16U();
        descriptor.resize(descriptorLength);
        stream.read(descriptor.data(), descriptorLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

bool Dcct::unpack(Common::ReadStream& stream) {
    try {
        if (!MmtTableBase::unpack(stream)) {
            return false;
        }

        uint16_t uint16 = stream.getBe16U();
        sectionSyntaxIndicator = (uint16 & 0b1000000000000000) >> 15;
        sectionLength = uint16 & 0b0000111111111111;
        dataTransmissionSessionId = stream.get8U();
        uint8_t reservedFutureUse1 = stream.get8U();
        uint8_t uint8 = stream.get8U();
        versionNumber = (uint8 & 0b00111110) >> 1;
        currentNextIndicator = uint8 & 1;
        sectionNumber = stream.get8U();
        lastSectionNumber = stream.get8U();

        contentId = stream.getBe16U();
        contentVersion = stream.get8U();
        contentSize = stream.getBe32U();

        uint8 = stream.get8U();
        puInfoFlag = (uint8 & 0b10000000) >> 7;
        contentInfoFlag = (uint8 & 0b01000000) >> 6;

        if (puInfoFlag) {
            uint16_t numOfPUs = stream.getBe16U();
            for (int i = 0; i < numOfPUs; i++) {
                PU pu;
                if (!pu.unpack(stream)) {
                    return false;
                }
                pus.push_back(pu);
            }
        }
        else {
            uint16_t numOfNodeTags = stream.getBe16U();
            for (int i = 0; i < numOfNodeTags; i++) {
                nodeTags.push_back(stream.getBe16U());
            }
        }

        if (contentInfoFlag) {
            uint16_t descriptorLength = stream.getBe16U();
            descriptor.resize(descriptorLength);
            stream.read(descriptor.data(), descriptorLength);
        }

        crc32 = stream.getBe32U();
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

}
