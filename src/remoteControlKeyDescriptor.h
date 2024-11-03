#pragma once
#include "tlvDescriptorBase.h"
#include <list>

namespace MmtTlv {

class RemoteControlKeyDescriptor : public TlvDescriptorTemplate<0xCD> {
public:
    bool unpack(Common::Stream& stream);
    
    class Entry {
    public:
        bool unpack(Common::Stream& stream);
        uint8_t remoteControlKeyId;
        uint16_t serviceId;

    };

    uint8_t numOfRemoteControlKeyId;
    std::list<Entry> entries;
};

}