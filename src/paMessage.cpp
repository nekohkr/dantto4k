#include "paMessage.h"

namespace MmtTlv {

bool PaMessage::unpack(Common::ReadStream& stream)
{
	try {
		messageId = stream.getBe16U();
		version = stream.get8U();
		length = stream.getBe32U();

		numberOfTables = stream.get8U();

		if (stream.leftBytes() < (8 + 8 + 16) * numberOfTables) {
			return false;
		}

		for (int i = 0; i < numberOfTables; i++) {
			stream.skip(8 + 8 + 16);
		}

		table.resize(stream.leftBytes());
		stream.read(table.data(), stream.leftBytes());
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

}