#include "mpu.h"

namespace MmtTlv {

bool Mpu::unpack(Common::Stream& stream)
{
	try {
		payloadLength = stream.getBe16U();
		if (payloadLength != stream.leftBytes())
			return false;

		uint8_t byte = stream.get8U();
		fragmentType = static_cast<FragmentType>(byte >> 4);
		timedFlag = (byte >> 3) & 1;
		fragmentationIndicator = static_cast<FragmentationIndicator>((byte >> 1) & 0b11);
		aggregateFlag = byte & 1;

		fragmentCounter = stream.get8U();
		mpuSequenceNumber = stream.getBe32U();

		payload.resize(payloadLength - 6);
		stream.read(payload.data(), payloadLength - 6);

	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

}