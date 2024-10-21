#pragma once
#include "mmtDescriptor.h"
#include <list>

class MhEventGroupDescriptor
    : public MmtDescriptor<0x800C> {
public:
    bool unpack(Stream& stream) override;

    uint8_t groupType;
    uint8_t eventCount;

    class Event {
    public:
        bool unpack(Stream& stream);
        uint16_t serviceId;
        uint16_t eventId;
    };

    class OtherNetworkEvent {
    public:
        bool unpack(Stream& stream);
        uint16_t originalNetworkId;
        uint16_t tlvStreamId;
        uint16_t serviceId;
        uint16_t eventId;
    };

    std::list<Event> events;
    std::list<OtherNetworkEvent> otherNetworkEvents;

    std::vector<uint8_t> privateDataByte;
};