#pragma once
#include "mmtDescriptor.h"
#include <list>



class MhContentDescriptor
    : public MmtDescriptor<0x8012> {
public:
    bool unpack(Stream& stream) override;

    class Entry {
    public:
        bool unpack(Stream& stream);

        uint8_t contentNibbleLevel1;
        uint8_t contentNibbleLevel2;
        uint8_t userNibble1;
        uint8_t userNibble2;
    };

    std::list<Entry> entries;
};