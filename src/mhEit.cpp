#include "mhEit.h"

namespace MmtTlv {

bool MhEit::unpack(Common::ReadStream& stream)
{
    try {
        if (!MmtTableBase::unpack(stream)) {
            return false;
        }

        uint16_t uint16 = stream.getBe16U();
        sectionSyntaxIndicator = (uint16 & 0b1000000000000000) >> 15;
        sectionLength = uint16 & 0b0000111111111111;

        serviceId = stream.getBe16U();

        uint8_t uint8 = stream.get8U();
        versionNumber = (uint8 & 0b00111110) >> 1;
        currentNextIndicator = uint8 & 1;

        sectionNumber = stream.get8U();
        lastSectionNumber = stream.get8U();
        tlvStreamId = stream.getBe16U();
        originalNetworkId = stream.getBe16U();
        segmentLastSectionNumber = stream.get8U();
        lastTableId = stream.get8U();

        while (stream.leftBytes() - 4 > 0) {
            std::shared_ptr<Event> event = std::make_shared<Event>();
            if (!event->unpack(stream)) {
                return false;
            }

            events.push_back(event);
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

bool MhEit::Event::unpack(Common::ReadStream& stream)
{
    try {
        eventId = stream.getBe16U();

        uint64_t uint64 = stream.getBe64U();
        startTime = (uint64 >> 24) & 0xFFFFFFFFFF;
        duration = uint64 & 0xFFFFFF;

        uint16_t uint16 = stream.getBe16U();
        runningStatus = (uint16 & 0b1110000000000000) >> 13;
        freeCaMode = (uint16 & 0b0001000000000000) >> 12;
        descriptorsLoopLength = uint16 & 0b0000111111111111;

        if (stream.leftBytes() < descriptorsLoopLength) {
            return false;
        }

        Common::ReadStream nstream(stream, descriptorsLoopLength);
        if (!descriptors.unpack(nstream)) {
            return false;
        }
        stream.skip(descriptorsLoopLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

}