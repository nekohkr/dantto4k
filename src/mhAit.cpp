#include "mhAit.h"

namespace MmtTlv {

bool MhAit::unpack(Common::ReadStream& stream) {
    try {
        if (!MmtTableBase::unpack(stream)) {
            return false;
        }

        uint16_t uint16 = stream.getBe16U();
        sectionSyntaxIndicator = (uint16 & 0b1000000000000000) >> 15;
        sectionLength = uint16 & 0b0000111111111111;

        applicationType = stream.getBe16U();

        uint8_t uint8 = stream.get8U();
        versionNumber = (uint8 & 0b00111110) >> 1;
        currentNextIndicator = uint8 & 1;
        sectionNumber = stream.get8U();
        lastSectionNumber = stream.get8U();

        uint16 = stream.getBe16U();
        commonDescriptorLength = uint16 & 0b0000111111111111;
        
        Common::ReadStream nstream(stream, commonDescriptorLength);
        if (!descriptors.unpack(nstream)) {
            return false;
        }

        stream.skip(commonDescriptorLength);
        
        uint16 = stream.getBe16U();
        applicationLoopLength = uint16 & 0b0000111111111111;

        Common::ReadStream applicationStream(stream, applicationLoopLength);
        while (applicationStream.leftBytes() > 0) {
            Application applicationIdentifier;
            if (!applicationIdentifier.unpack(applicationStream)) {
                return false;
            }

            applications.push_back(std::move(applicationIdentifier));
        }
        stream.skip(applicationLoopLength);

        crc32 = stream.getBe32U();
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

bool MhAit::Application::unpack(Common::ReadStream& stream) {
    try {
        if (!applicationIdentifier.unpack(stream)) {
            return false;
        }

        applicationControlCode = stream.get8U();

        uint16_t uint16 = stream.getBe16U();
        applicationDescriptorLoopLength = uint16 & 0b0000111111111111;
        
        Common::ReadStream nstream(stream, applicationDescriptorLoopLength);
        if (!descriptors.unpack(nstream)) {
            return false;
        }

        stream.skip(applicationDescriptorLoopLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

bool MhAit::ApplicationIdentifier::unpack(Common::ReadStream& stream) {
    try {
        organizationId = stream.getBe16U();
        applicationId = stream.getBe32U();
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

}