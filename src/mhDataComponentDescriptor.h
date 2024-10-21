#pragma once
#include "mmtDescriptor.h"

class MhDataComponentDescriptor
    : public MmtDescriptor<0x8020> {
public:
    bool unpack(Stream& stream) override;

    uint16_t dataComponentId;
    std::vector<uint8_t> additionalDataComponentInfo;
};