#pragma once
#include "stream.h"

namespace MmtTlv {

class TlvTableId {
public:
	static constexpr uint8_t Nit = 0x40;
};

class TlvTableBase {
public:
    virtual ~TlvTableBase() = default;

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