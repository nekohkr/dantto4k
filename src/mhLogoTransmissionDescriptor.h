#pragma once
#include "mmtDescriptor.h"
#include <list>

class MhLogoTransmissionDescriptor
    : public MmtDescriptor<0x8025> {
public:
    bool unpack(Stream& stream) override;

    class Entry {
    public:
        bool unpack(Stream& stream);
        uint8_t logoType;
        uint8_t startSectionNumber;
        uint8_t numOfSections;
    };

    uint8_t logoTransmissionType;

    uint8_t reservedFutureUse1; //7 bits
    uint16_t logoId; //9 bits
    uint8_t reservedFutureUse2; //4 bits
    uint16_t logoVersion; //12 bits
    uint16_t downloadDataId; //16 bits

    std::list<Entry> entries;

    std::string logoChar;
};
