#pragma once
#include "mmtDescriptorBase.h"

namespace MmtTlv {

class EventPackageDescriptor
    : public MmtDescriptorTemplate<0x8001> {
public:
    bool unpack(Common::ReadStream& stream) override;

    uint8_t mmtPackageIdLength;
    std::vector<uint8_t> mmtPackageIdByte;
};

}