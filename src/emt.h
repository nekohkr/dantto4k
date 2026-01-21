#pragma once
#include "mmtTableBase.h"
#include <list>
#include <optional>
#include "mmtDescriptors.h"

namespace MmtTlv {

// Event Message Table
class Emt : public MmtTableBase {
public:
    bool unpack(Common::ReadStream& stream);

    bool sectionSyntaxIndicator;
    uint16_t sectionLength;
    uint8_t dataEventId;
    uint16_t eventMsgGroupId;
    uint8_t versionNumber;
    bool currentNextIndicator;
    uint8_t sectionNumber;
    uint8_t lastSectionNumber;
    MmtDescriptors descriptors;
    uint32_t crc32;

};

}