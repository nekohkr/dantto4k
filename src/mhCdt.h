#pragma once
#include "mmtTable.h"
#include "mmtp.h"
#include "mmtDescriptors.h"
#include <list>

class MhCdt : public MmtTable {
public:
    bool unpack(Stream& stream);

    uint16_t sectionSyntaxIndicator;
    uint16_t sectionLength;

    uint16_t downloadDataId;
    uint8_t versionNumber;
    bool currentNextIndicator;
    uint8_t sectionNumber;
    uint8_t lastSectionNumber;
    uint16_t originalNetworkId;

    uint8_t dataType;

    uint8_t reservedFutureUse;
    uint16_t descriptorsLoopLength;

    MmtDescriptors descriptors;


    std::vector<uint8_t> dataModuleByte;
    uint32_t crc32;
};