#include "mmtTable.h"

bool MmtTable::unpack(Stream& stream)
{
	if (stream.leftBytes() < 1) {
		return false;
	}

    tableId = stream.get8U();

    return true;
}
