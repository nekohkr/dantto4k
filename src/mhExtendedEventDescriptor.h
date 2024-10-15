#pragma once
#include "mmtDescriptor.h"
#include <list>

class MhExtendedEventDescriptor
    : public MmtDescriptor<0xF002, true> {
public:
    bool unpack(Stream& stream) override;

    class Entry {
    public:
        bool unpack(Stream& stream);

        uint8_t itemDescriptionLength;
        std::string itemDescriptionChar;

        uint16_t itemLength;
        std::string itemChar;
    };

    uint8_t descriptorNumber;
    uint8_t lastDescriptorNumber;
    char language[4];
    uint16_t lengthOfItems;

    std::list<Entry> entries;

    uint16_t textLength;
    std::string textChar;
};