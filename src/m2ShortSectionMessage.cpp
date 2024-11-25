#include "m2ShortSectionMessage.h"

namespace MmtTlv {

bool M2ShortSectionMessage::unpack(Common::ReadStream& stream)
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