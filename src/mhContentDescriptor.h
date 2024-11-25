#pragma once
#include "mmtDescriptorBase.h"
#include <list>

namespace MmtTlv {

class MhContentDescriptor
    : public MmtDescriptorTemplate<0x8012> {
public:
    bool unpack(Common::ReadStream& stream) override;

    class Entry {
    public:
        bool unpack(Common::ReadStream& stream);

        uint8_t contentNibbleLevel1;
        uint8_t contentNibbleLevel2;
        uint8_t userNibble1;
        uint8_t userNibble2;
    };

    std::list<Entry> entries;
};

}