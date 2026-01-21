#pragma once
#include "mmtDescriptorBase.h"
#include <list>

namespace MmtTlv {

class MhRandomizedLatencyDescriptor
    : public MmtDescriptorTemplate<0x802F> {
public:
    bool unpack(Common::ReadStream& stream) override;

    uint16_t range;
    uint8_t rate;
    bool randomizationEndTimeFlag;
    uint8_t reserved;
    uint64_t randomizationEndTime;

};

}