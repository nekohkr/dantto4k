#pragma once
#include "mmtDescriptor.h"
#include <list>
class MhCaContractInformation
    : public MmtDescriptor<0x8041> {
public:
    bool unpack(Stream& stream) override;

    uint16_t caSystemId;
    uint8_t caUnitId; //4 bits
    uint8_t numOfComponent; //4 bits

    std::list<uint16_t> componentTags;

    uint8_t contractVerificationInfoLength;
    std::vector<uint8_t> contractVerificationInfo;

    uint8_t feeNameLength;
    std::string feeName;

};
