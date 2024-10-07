#pragma once
#include "mmtTable.h"

class Ecm : public MmtTable {
public:
    bool unpack(Stream& stream);

    uint16_t sectionSyntaxIndicator;
    uint16_t sectionLength;
    uint16_t tlvStreamId;

    bool currentNextIndicator;
    uint8_t sectionNumber;
    uint8_t lastSectionNumber;

    std::vector<uint8_t> ecmData;

    uint32_t crc32;
};