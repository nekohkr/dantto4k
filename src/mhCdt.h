#pragma once
#include <vector>
#include "mmtTableBase.h"
#include "mmtDescriptors.h"

namespace MmtTlv {

// Mh-Common Data Table
class MhCdt : public MmtTableBase {
public:
    bool unpack(Common::Stream& stream);

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

}