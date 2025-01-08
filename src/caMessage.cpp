#include "caMessage.h"

namespace MmtTlv {

bool CaMessage::unpack(Common::ReadStream& stream)
{
	try {
		messageId = stream.getBe16U();
		version = stream.get8U();
		length = stream.getBe16U();
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

}