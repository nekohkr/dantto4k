#include "mhEit.h"

#include "videoComponentDescriptor.h"
#include "mhAudioComponentDescriptor.h"
#include "mhShortEventDescriptor.h"
#include "mhExtendedEventDescriptor.h"
#include "mhContentDescriptor.h"

bool MhEit::unpack(Stream& stream)
{
    try {
        if (!MmtTable::unpack(stream)) {
            return false;
        }

        uint16_t uint16 = stream.getBe16U();
        sectionSyntaxIndicator = (uint16 & 0b1000000000000000) >> 15;
        sectionLength = uint16 & 0b0000111111111111;

        serviceId = stream.getBe16U();

        uint8_t uint8 = stream.get8U();
        currentNextIndicator = uint8 & 1;

        sectionNumber = stream.get8U();
        lastSectionNumber = stream.get8U();
        tlvStreamId = stream.getBe16U();
        originalNetworkId = stream.getBe16U();
        segmentLast_sectionNumber = stream.get8U();
        lastTableId = stream.get8U();

        while (stream.leftBytes() - 4 > 0) {
            MHEvent* event = new MHEvent();
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

MHEvent::~MHEvent()
{
    for (auto descriptor : descriptors) {
        delete descriptor;
    }
    descriptors.clear();
}

bool MHEvent::unpack(Stream& stream)
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

        Stream nstream(stream, descriptorsLoopLength);
        while (!nstream.isEOF()) {
            uint16_t descriptorTag = nstream.peekBe16U();
            switch (descriptorTag) {
            case VIDEO_COMPONENT_DESCRIPTOR:
            {
                VideoComponentDescriptor* descriptor = new VideoComponentDescriptor();
                if (!descriptor->unpack(nstream)) {
                    return false;
                }
                descriptors.push_back(descriptor);
                break;
            }
            case MH_AUDIO_COMPONENT_DESCRIPTOR:
            {
                MhAudioComponentDescriptor* descriptor = new MhAudioComponentDescriptor();
                if (!descriptor->unpack(nstream)) {
                    return false;
                }
                descriptors.push_back(descriptor);
                break;
            }
            case MH_SHORT_EVENT_DESCRIPTOR:
            {
                MhShortEventDescriptor* descriptor = new MhShortEventDescriptor();
                if (!descriptor->unpack(nstream)) {
                    return false;
                }
                descriptors.push_back(descriptor);
                break;
            }
            case MH_EXTENDED_EVENT_DESCRIPTOR:
            {
                MhExtendedEventDescriptor* descriptor = new MhExtendedEventDescriptor();
                if (!descriptor->unpack(nstream)) {
                    return false;
                }
                descriptors.push_back(descriptor);
                break;
            }
            case MH_CONTENT_DESCRIPTOR:
            {
                MhContentDescriptor* descriptor = new MhContentDescriptor();
                if (!descriptor->unpack(nstream)) {
                    return false;
                }
                descriptors.push_back(descriptor);
                break;
            }
            default:
            {
                MmtDescriptor* descriptor = new MmtDescriptor();
                descriptor->unpack(nstream);
                nstream.skip(descriptor->descriptorLength);
                break;
            }
            }
        }

        stream.skip(descriptorsLoopLength);

    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}