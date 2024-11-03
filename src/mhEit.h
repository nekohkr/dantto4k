#pragma once
#include <list>
#include "mmtTableBase.h"
#include "mmtDescriptors.h"

namespace MmtTlv {

class MmtDescriptorBase;

// Mh-Event Information Table
class MhEit : public MmtTableBase {
public:
    bool unpack(Common::Stream& stream);

    class Event {
    public:
        bool unpack(Common::Stream& stream);

        uint16_t eventId;
        int64_t startTime;
        uint32_t duration;
        uint8_t runningStatus;
        uint8_t freeCaMode;
        uint16_t descriptorsLoopLength;

        MmtDescriptors descriptors;
    };

    bool isPF() const {
        return tableId == MmtTableId::MhEit ? true : false;
    }

    uint16_t sectionSyntaxIndicator;
    uint16_t sectionLength;

    uint16_t serviceId;
    bool currentNextIndicator;
    uint8_t sectionNumber;
    uint8_t lastSectionNumber;
    uint16_t tlvStreamId;
    uint16_t originalNetworkId;
    uint8_t segmentLastSectionNumber;
    uint8_t lastTableId;
    uint8_t eventCount;

    std::list<std::shared_ptr<Event>> events;
    uint32_t crc32;
};

}