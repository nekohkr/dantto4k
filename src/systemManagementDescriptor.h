#pragma once
#include "tlvDescriptorBase.h"
#include <list>

namespace MmtTlv {

class SystemManagementDescriptor : public TlvDescriptorTemplate<0xFE> {
public:
    bool unpack(Common::Stream& stream);

    uint16_t systemManagementId;
    std::vector<uint8_t> additionalIdentificationInfo;
};

}