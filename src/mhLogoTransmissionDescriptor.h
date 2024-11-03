#pragma once
#include "mmtDescriptorBase.h"
#include <list>

namespace MmtTlv {

class MhLogoTransmissionDescriptor
    : public MmtDescriptorTemplate<0x8025> {
public:
    bool unpack(Common::Stream& stream) override;

    class Entry {
    public:
        bool unpack(Common::Stream& stream);
        uint8_t logoType;
        uint8_t startSectionNumber;
        uint8_t numOfSections;
    };

    uint8_t logoTransmissionType;

    uint8_t reservedFutureUse1;
    uint16_t logoId;
    uint8_t reservedFutureUse2;
    uint16_t logoVersion;
    uint16_t downloadDataId;

    std::list<Entry> entries;

    std::string logoChar;
};

}