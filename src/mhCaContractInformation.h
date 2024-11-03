#pragma once
#include "mmtDescriptorBase.h"
#include <list>

namespace MmtTlv {

class MhCaContractInformation
    : public MmtDescriptorTemplate<0x8041> {
public:
    bool unpack(Common::Stream& stream) override;

    uint16_t caSystemId;
    uint8_t caUnitId;
    uint8_t numOfComponent;

    std::list<uint16_t> componentTags;

    uint8_t contractVerificationInfoLength;
    std::vector<uint8_t> contractVerificationInfo;

    uint8_t feeNameLength;
    std::string feeName;

};

}