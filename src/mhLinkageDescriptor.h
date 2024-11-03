#pragma once
#include "mmtDescriptorBase.h"

namespace MmtTlv {

class MhLinkageDescriptor
    : public MmtDescriptorTemplate<0xF000> {
public:
    bool unpack(Common::Stream& stream) override;

    uint16_t tlvStreamId;
    uint16_t originalNetworkId;
    uint16_t serviceId;
    uint8_t linkageType;
    std::vector<uint8_t> privateDataByte;

};

}