#include "mmtTable.h"

bool MmtTable::unpack(Stream& stream)
{
    try {
		tableId = stream.get8U();
	}
	catch (const std::out_of_range&) {
		return false;
	}

    return true;
}
