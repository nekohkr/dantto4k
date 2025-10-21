#pragma once
#include "stream.h"

namespace MmtTlv {
    
namespace MmtTableId {

constexpr uint8_t Pat = 0x00;
constexpr uint8_t Mpt = 0x20;
constexpr uint8_t Plt = 0x80;
constexpr uint8_t Lct = 0x81;
constexpr uint8_t Ecm_0 = 0x82;
constexpr uint8_t Ecm_1 = 0x83;
constexpr uint8_t Emm_0 = 0x84;
constexpr uint8_t Emm_1 = 0x85;
constexpr uint8_t Cat = 0x86;
constexpr uint8_t Dcm = 0x87;
constexpr uint8_t Dmm = 0x89;
constexpr uint8_t MhEitPf = 0x8B; // present and next program
constexpr uint8_t MhEitS_0 = 0x8C; // schedule
constexpr uint8_t MhEitS_1 = 0x8D;
constexpr uint8_t MhEitS_2 = 0x8E;
constexpr uint8_t MhEitS_3 = 0x8F;
constexpr uint8_t MhEitS_4 = 0x90;
constexpr uint8_t MhEitS_5 = 0x91;
constexpr uint8_t MhEitS_6 = 0x92;
constexpr uint8_t MhEitS_7 = 0x93;
constexpr uint8_t MhEitS_8 = 0x94;
constexpr uint8_t MhEitS_9 = 0x95;
constexpr uint8_t MhEitS_10 = 0x96;
constexpr uint8_t MhEitS_11 = 0x97;
constexpr uint8_t MhEitS_12 = 0x98;
constexpr uint8_t MhEitS_13 = 0x99;
constexpr uint8_t MhEitS_14 = 0x9A;
constexpr uint8_t MhEitS_15 = 0x9B;
constexpr uint8_t MhAit = 0x9C;
constexpr uint8_t MhBit = 0x9D;
constexpr uint8_t MhSdtt = 0x9E;
constexpr uint8_t MhSdtActual = 0x9F;
constexpr uint8_t MhSdtOther = 0xA0;
constexpr uint8_t MhTot = 0xA1;
constexpr uint8_t MhCdt = 0xA2;
constexpr uint8_t Ddmt = 0xA3;
constexpr uint8_t Damt = 0xA4;
constexpr uint8_t Dcct = 0xA5;
constexpr uint8_t Emt = 0xA6;
	
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