#pragma once
#include "mmtTable.h"
#include "mmtp.h"
#include <list>

class MmtDescriptor;
class MHEvent {
public:
    ~MHEvent();
    bool unpack(Stream& stream);

    uint16_t eventId;
    int64_t startTime;
    uint32_t duration;
    uint8_t runningStatus;
    uint8_t freeCaMode;
    uint16_t descriptorsLoopLength;

    std::list<MmtDescriptor*> descriptors;
};

class MhEit : public MmtTable {
public:
    bool unpack(Stream& stream);

    uint16_t sectionSyntaxIndicator;
    uint16_t sectionLength;

    uint16_t serviceId;
    bool currentNextIndicator;
    uint8_t sectionNumber;
    uint8_t lastSectionNumber;
    uint16_t tlvStreamId;
    uint16_t originalNetworkId;
    uint8_t segmentLast_sectionNumber;
    uint8_t lastTableId;
    uint8_t eventCount;

    std::list<MHEvent*> events;
    uint32_t crc32;
};