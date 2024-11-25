#pragma once
#include "mmtTableBase.h"

namespace MmtTlv {

// Entitlement Control Message
class Ecm : public MmtTableBase {
public:
    bool unpack(Common::ReadStream& stream);

    uint16_t sectionSyntaxIndicator;
    uint16_t sectionLength;
    uint16_t tlvStreamId;

    bool currentNextIndicator;
    uint8_t sectionNumber;
    uint8_t lastSectionNumber;

    std::vector<uint8_t> ecmData;

    uint32_t crc32;
};

}