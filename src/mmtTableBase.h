#pragma once
#include "stream.h"

namespace MmtTlv {
    
class MmtTableId {
public:
	static constexpr uint8_t Pat = 0x00;
	static constexpr uint8_t Mpt = 0x20;
	static constexpr uint8_t Plt = 0x80;
	static constexpr uint8_t Lct = 0x81;
	static constexpr uint8_t Ecm_0 = 0x82;
	static constexpr uint8_t Ecm_1 = 0x83;
	static constexpr uint8_t Emm_0 = 0x84;
	static constexpr uint8_t Emm_1 = 0x85;
	static constexpr uint8_t Cat = 0x86;
	static constexpr uint8_t Dcm = 0x87;
	static constexpr uint8_t Dmm = 0x89;
	static constexpr uint8_t MhEitPf = 0x8B; // present and next program
    static constexpr uint8_t MhEitS_0 = 0x8C; // schedule
    static constexpr uint8_t MhEitS_1 = 0x8D;
    static constexpr uint8_t MhEitS_2 = 0x8E;
    static constexpr uint8_t MhEitS_3 = 0x8F;
    static constexpr uint8_t MhEitS_4 = 0x90;
    static constexpr uint8_t MhEitS_5 = 0x91;
    static constexpr uint8_t MhEitS_6 = 0x92;
    static constexpr uint8_t MhEitS_7 = 0x93;
    static constexpr uint8_t MhEitS_8 = 0x94;
    static constexpr uint8_t MhEitS_9 = 0x95;
    static constexpr uint8_t MhEitS_10 = 0x96;
    static constexpr uint8_t MhEitS_11 = 0x97;
    static constexpr uint8_t MhEitS_12 = 0x98;
    static constexpr uint8_t MhEitS_13 = 0x99;
    static constexpr uint8_t MhEitS_14 = 0x9A;
    static constexpr uint8_t MhEitS_15 = 0x9B;
	static constexpr uint8_t MhAit = 0x9C;
	static constexpr uint8_t MhBit = 0x9D;
	static constexpr uint8_t MhSdtt = 0x9E;
	static constexpr uint8_t MhSdtActual = 0x9F;
	static constexpr uint8_t MhSdtOther = 0xA0;
	static constexpr uint8_t MhTot = 0xA1;
	static constexpr uint8_t MhCdt = 0xA2;

	static constexpr uint8_t Ddmt = 0xA3;
	static constexpr uint8_t Damt = 0xA4;
	static constexpr uint8_t Dcct = 0xA5;

	static constexpr uint8_t Emt = 0xA6;
	
};

class MmtTableBase {
public:
    virtual ~MmtTableBase() = default;

    virtual bool unpack(Common::ReadStream& stream)
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