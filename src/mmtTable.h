#pragma once
#include "stream.h"

class MmtTable {
public:
    virtual ~MmtTable() {}

    bool unpack(Stream& stream);
    uint8_t getTableId() const { return tableId; }

protected:
    uint8_t tableId;

};