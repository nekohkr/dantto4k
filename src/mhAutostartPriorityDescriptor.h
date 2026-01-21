#pragma once
#include "mmtDescriptorBase.h"

namespace MmtTlv {

class MhAutostartPriorityDescriptor
    : public MmtDescriptorTemplate<0x802D> {
public:
    bool unpack(Common::ReadStream& stream) override;

    uint8_t autostartPriority;

};

}