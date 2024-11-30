#include "mhSdt.h"
#include "mhServiceDescriptor.h"

namespace MmtTlv {

bool MhSdt::unpack(Common::ReadStream& stream)
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
        versionNumber = (uint8 & 0b00111110) >> 1;
        currentNextIndicator = uint8 & 1;
        sectionNumber = stream.get8U();
        lastSectionNumber = stream.get8U();
        originalNetworkId = stream.getBe16U();
        stream.skip(1);

        while (stream.leftBytes() > 4) {
            std::shared_ptr<Service> service = std::make_shared<Service>();
            if (!service->unpack(stream)) {
                return false;
            }

            services.push_back(service);
        }

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

bool MhSdt::Service::unpack(Common::ReadStream& stream)
{
    try {

        serviceId = stream.getBe16U();

        uint8_t uint8 = stream.get8U();
        eitUserDefinedFlags = (uint8 & 0b00011100) >> 2;
        eitScheduleFlag = (uint8 & 0b00000010) >> 1;
        eitPresentFollowingFlag = (uint8 & 0b00000001) >> 1;

        uint16_t uint16 = stream.getBe16U();
        runningStatus = (uint16 & 0b1110000000000000) >> 13;
        freeCaMode = (uint16 & 0b0001000000000000) >> 12;
        descriptorsLoopLength = uint16 & 0b0000111111111111;

        Common::ReadStream nstream(stream, descriptorsLoopLength);
        descriptors.unpack(nstream);
        stream.skip(descriptorsLoopLength);
	}
	catch (const std::out_of_range&) {
		return false;
	}

    return true;
}

}