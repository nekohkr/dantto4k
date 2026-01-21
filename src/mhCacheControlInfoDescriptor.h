#pragma once
#include "mmtDescriptorBase.h"

namespace MmtTlv {

class MhCacheControlInfoDescriptor
    : public MmtDescriptorTemplate<0x802E> {
public:
    bool unpack(Common::ReadStream& stream) override;

    uint16_t applicationSize;
    uint8_t cachePriority;
    bool packageFlag;
    uint8_t applicationVersion;
    uint16_t expireDate;
};

}