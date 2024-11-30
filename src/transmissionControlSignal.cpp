#include "transmissionControlSignal.h"

namespace MmtTlv {

bool TransmissionControlSignal::unpack(Common::ReadStream& stream)
{
	try {
		tableId = stream.get8U();
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

}