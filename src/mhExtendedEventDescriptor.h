#pragma once
#include "mmtDescriptor.h"
#include <list>

class MhExtendedEventItem {
public:
    bool unpack(Stream& stream);

    uint8_t itemDescriptionLength;
    std::string itemDescriptionChar;
    
    uint16_t itemLength;
    std::string itemChar;
};

class MhExtendedEventDescriptor : public MmtDescriptor {
public:
    bool unpack(Stream& stream);

    uint8_t descriptorNumber;
    uint8_t lastDescriptorNumber;
    char language[4];
    uint16_t lengthOfItems;

    std::list<MhExtendedEventItem> items;

    uint16_t textLength;
    std::string textChar;
};