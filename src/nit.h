#pragma once
#include <list>
#include "tlvTableBase.h"
#include "tlvDescriptors.h"
#include "stream.h"

namespace MmtTlv {

// Network Information Table
class Nit : public TlvTableBase {
public:
    bool unpack(Common::Stream& stream);
    
    class Entry {
    public:
        bool unpack(Common::Stream& stream);

        uint16_t tlvStreamId;
        uint16_t originalNetworkId;
        uint16_t tlvStreamDescriptorsLength;
        TlvDescriptors descriptors;
    };

    
    uint16_t sectionSyntaxIndicator;
    uint16_t sectionLength;
    uint16_t networkId;
    bool currentNextIndicator;
    uint8_t sectionNumber;
    uint8_t lastSectionNumber;

    uint16_t networkDescriptorsLength;
    TlvDescriptors descriptors;

    uint16_t tlvStreamLoopLength;
    std::list<Entry> entries;

};

}