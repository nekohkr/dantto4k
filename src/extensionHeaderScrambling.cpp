#include "extensionHeaderScrambling.h"

namespace MmtTlv {

bool ExtensionHeaderScrambling::unpack(Common::Stream& stream, uint16_t extensionHeaderType, uint16_t extensionHeaderLength)
{
	try {
		if (stream.leftBytes() < 1) {
			return false;
		}

		uint8_t uint8 = stream.get8U();
		encryptionFlag = static_cast<EncryptionFlag>((uint8 & 0b00011000) >> 3);
		scramblingSubsystem = (uint8 & 0b00000100) >> 2;
		messageAuthenticationControl = (uint8 & 0b00000010) >> 1;
		scramblingInitialCounterValue = uint8 & 0b00000001;
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

}