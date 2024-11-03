#include "transmissionControlSignal.h"

namespace MmtTlv {

bool TransmissionControlSignal::unpack(Common::Stream& stream)
{
	tableId = stream.get8U();
	return true;
}

}