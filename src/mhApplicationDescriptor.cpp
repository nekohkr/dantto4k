#include "mhApplicationDescriptor.h"

namespace MmtTlv {

bool MhApplicationDescriptor::unpack(Common::ReadStream& stream)
{
    try {
        if (!MmtDescriptorTemplate::unpack(stream)) {
            return false;
        }

        Common::ReadStream nstream(stream, descriptorLength);

        applicationProfilesLength = nstream.get8U();
        size_t leftBytes = nstream.leftBytes();
        while (nstream.leftBytes() - (leftBytes - applicationProfilesLength) > 0) {
            ApplicationProfile applicationProfile;
            if (!applicationProfile.unpack(nstream)) {
                return false;
            }

            applicationProfiles.push_back(applicationProfile);
        }

        uint8_t uint8 = nstream.get8U();
        
        serviceBoundFlag = (uint8 & 0b10000000) >> 7;
        visibility = (uint8 & 0b01100000) >> 5;
        presentApplicationPriority = uint8 & 0b00000001;

        applicationPriority = nstream.get8U();

        transportProtocolLabel.resize(nstream.leftBytes());
        nstream.read(transportProtocolLabel.data(), nstream.leftBytes());

        stream.skip(descriptorLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

bool MhApplicationDescriptor::ApplicationProfile::unpack(Common::ReadStream& stream)
{
    try {
        applicationProfile = stream.getBe16U();
        versionMajor = stream.get8U();
        versionMinor = stream.get8U();
        versionMicro = stream.get8U();
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

}