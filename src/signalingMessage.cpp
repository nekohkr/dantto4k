#include "signalingMessage.h"
#include "fragmentAssembler.h"

namespace MmtTlv {

bool SignalingMessage::unpack(Common::ReadStream& stream)
{
	try {
		uint8_t uint8 = stream.get8U();
		fragmentationIndicator = static_cast<FragmentationIndicator>((uint8 & 0b11000000) >> 6);
		reserved = (uint8 & 0b00111100) >> 2;
		lengthExtensionFlag = (uint8 & 0x00000010) >> 2;
		aggregationFlag = uint8 & 1;

		fragmentCounter = stream.get8U();

		payload.resize(stream.leftBytes());
		stream.read(payload.data(), stream.leftBytes());
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

}