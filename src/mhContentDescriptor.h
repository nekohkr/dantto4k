#pragma once
#include "mmtDescriptor.h"
#include <list>

class MhContentItem {
public:
    bool unpack(Stream& stream);

    uint8_t contentNibbleLevel1;
    uint8_t contentNibbleLevel2;
    uint8_t userNibble1;
    uint8_t userNibble2;
};

class MhContentDescriptor : public MmtDescriptor {
public:
    bool unpack(Stream& stream);

    std::list<MhContentItem> items;

};