#pragma once
#include "mmtDescriptorBase.h"
#include <list>

namespace MmtTlv {

class MhParentalRatingDescriptor
    : public MmtDescriptorTemplate<0x8013> {
public:
    bool unpack(Common::ReadStream& stream) override;

    class Entry {
    public:
        bool unpack(Common::ReadStream& stream);

        char countryCode[4];
        uint8_t rating;
    };
    
    std::list<Entry> entries;
};

}