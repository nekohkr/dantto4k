#include "dataTransmissionMessage.h"

namespace MmtTlv {

bool DataTransmissionMessage::unpack(Common::ReadStream& stream)
{
	try {
		messageId = stream.getBe16U();
		version = stream.get8U();
		length = stream.getBe32U();
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

}