#pragma once
#include "mmtTableBase.h"

namespace MmtTlv {

// Mh-Time Offset Table
class MhTot : public MmtTableBase {
public:
    bool unpack(Common::Stream& stream);

    uint16_t sectionSyntaxIndicator;
    uint16_t sectionLength;
    uint64_t jstTime;
};

}