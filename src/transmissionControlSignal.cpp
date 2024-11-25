#include "transmissionControlSignal.h"

namespace MmtTlv {

bool TransmissionControlSignal::unpack(Common::ReadStream& stream)
{
	tableId = stream.get8U();
	return true;
}

}