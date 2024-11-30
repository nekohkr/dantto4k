#pragma once
#include "mmtDescriptorBase.h"

namespace MmtTlv {

class MhBroadcasterNameDescriptor
    : public MmtDescriptorTemplate<0x8018> {
public:
    bool unpack(Common::ReadStream& stream) override;

    std::string text;
};

}