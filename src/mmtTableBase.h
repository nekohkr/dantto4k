#pragma once
#include "stream.h"

namespace MmtTlv {
    
class MmtTableId {
public:
	static constexpr uint8_t Mpt = 0x20;
	static constexpr uint8_t Plt = 0x80;
	static constexpr uint8_t Ecm = 0x82;
	static constexpr uint8_t MhEitPf = 0x8B; //present and next program
    static constexpr uint8_t MhEitS_1 = 0x8C; //schedule
    static constexpr uint8_t MhEitS_2 = 0x8D;
    static constexpr uint8_t MhEitS_3 = 0x8E;
    static constexpr uint8_t MhEitS_4 = 0x8F;
    static constexpr uint8_t MhEitS_5 = 0x90;
    static constexpr uint8_t MhEitS_6 = 0x91;
    static constexpr uint8_t MhEitS_7 = 0x92;
    static constexpr uint8_t MhEitS_8 = 0x93;
    static constexpr uint8_t MhEitS_9 = 0x94;
    static constexpr uint8_t MhEitS_10 = 0x95;
    static constexpr uint8_t MhEitS_11 = 0x96;
    static constexpr uint8_t MhEitS_12 = 0x97;
    static constexpr uint8_t MhEitS_13 = 0x98;
    static constexpr uint8_t MhEitS_14 = 0x99;
    static constexpr uint8_t MhEitS_15 = 0x9A;
    static constexpr uint8_t MhEitS_16 = 0x9B;
	static constexpr uint8_t MhSdt = 0x9F;
	static constexpr uint8_t MhTot = 0xA1;
	static constexpr uint8_t MhCdt = 0xA2;
	static constexpr uint8_t MhBit = 0x9D;
};

class MmtTableBase {
public:
    virtual ~MmtTableBase() = default;

    virtual bool unpack(Common::Stream& stream)
    {
	    try {
		    tableId = stream.get8U();
	    }
	    catch (const std::out_of_range&) {
		    return false;
	    }

	    return true;
    }
    uint8_t getTableId() const { return tableId; }

protected:
    uint8_t tableId;

};

}