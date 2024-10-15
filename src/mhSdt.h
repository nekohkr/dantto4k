#pragma once
#include "mmtTable.h"
#include "mmtp.h"
#include "mmtDescriptors.h"
#include <list>

class MmtDescriptorBase;

class MhSdtService {
public:
    virtual ~MhSdtService();
    bool unpack(Stream& stream);

    uint16_t serviceId;
    int8_t eitUserDefinedFlags;
    bool eitScheduleFlag;
    bool eitPresentFollowingFlag;
    uint8_t runningStatus;
    bool freeCaMode;
    uint16_t descriptorsLoopLength;

    MmtDescriptors descriptors;
};

class MhSdt : public MmtTable {
public:
    virtual ~MhSdt();
    bool unpack(Stream& stream);

    uint16_t sectionSyntaxIndicator;
    uint16_t sectionLength;

    uint16_t tlvStreamId;
    bool currentNextIndicator;
    uint8_t sectionNumber;
    uint8_t lastSectionNumber;

    uint16_t originalNetworkId;

    std::list<MhSdtService*> services;
    uint32_t crc32;
};