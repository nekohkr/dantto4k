#pragma once
#include "tlvDescriptorBase.h"
#include <list>

namespace MmtTlv {

class RemoteControlKeyDescriptor : public TlvDescriptorTemplate<0xCD> {
public:
    bool unpack(Common::ReadStream& stream);
    
    class Entry {
    public:
        bool unpack(Common::ReadStream& stream);
        uint8_t remoteControlKeyId;
        uint16_t serviceId;
        uint16_t reserved;

    };

    uint8_t numOfRemoteControlKeyId;
    std::list<Entry> entries;
};

}