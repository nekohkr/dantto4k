#pragma once
#include "tlvDescriptorBase.h"
#include <list>

namespace MmtTlv {

class ServiceListDescriptor : public TlvDescriptorTemplate<0x41> {
public:
    bool unpack(Common::ReadStream& stream);

    class Entry {
    public:
        bool unpack(Common::ReadStream& stream);
        uint16_t serviceId;
        uint8_t serviceType;
    };

    std::list<Entry> services;
};

}