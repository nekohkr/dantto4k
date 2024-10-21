#pragma once
#include "mmtDescriptor.h"
#include <list>

class MhParentalRatingDescriptor
    : public MmtDescriptor<0x8013> {
public:
    bool unpack(Stream& stream) override;

    class Entry {
    public:
        bool unpack(Stream& stream);

        char countryCode[4];
        uint8_t rating;
    };
    
    std::list<Entry> entries;
};
