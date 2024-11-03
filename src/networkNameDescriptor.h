#pragma once
#include "tlvDescriptorBase.h"

namespace MmtTlv {

class NetworkNameDescriptor : public TlvDescriptorTemplate<0x40> {
public:
    bool unpack(Common::Stream& stream);
    std::string networkName;
};

}