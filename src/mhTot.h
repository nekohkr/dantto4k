#pragma once
#include "mmtTable.h"

class MhTot : public MmtTable {
public:
    bool unpack(Stream& stream);

    uint16_t sectionSyntaxIndicator;
    uint16_t sectionLength;
    uint64_t jstTime;
};