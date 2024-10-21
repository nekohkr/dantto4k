#pragma once
#include "mmtDescriptor.h"

class EventPackageDescriptor
    : public MmtDescriptor<0x8001> {
public:
    bool unpack(Stream& stream) override;

    uint8_t mmtPackageIdLength;
    std::vector<uint8_t> mmtPackageIdByte;
};
