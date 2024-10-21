#pragma once
#include "mmtDescriptor.h"

class MhLinkageDescriptor
    : public MmtDescriptor<0xF000> {
public:
    bool unpack(Stream& stream) override;

    uint16_t tlvStreamId;
    uint16_t originalNetworkId;
    uint16_t serviceId;
    uint8_t linkageType;
    std::vector<uint8_t> privateDataByte;

};
