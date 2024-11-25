#pragma once
#include "mmtDescriptorBase.h"

namespace MmtTlv {

class MhDataComponentDescriptor
    : public MmtDescriptorTemplate<0x8020> {
public:
    bool unpack(Common::ReadStream& stream) override;

    uint16_t dataComponentId;
    std::vector<uint8_t> additionalDataComponentInfo;
};

}