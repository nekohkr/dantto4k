#pragma once
#include <cstdint>
#include "stream.h"

namespace MmtTlv {

enum class EncryptionFlag : uint8_t
{
	UNSCRAMBLED = 0x00,
	RESERVED = 0x01,
	EVEN = 0x02,
	ODD = 0x03,
};

class ExtensionHeaderScrambling {
public:
	bool unpack(Common::Stream& stream, uint16_t extensionHeaderType, uint16_t extensionHeaderLength);
	EncryptionFlag encryptionFlag;
	uint8_t scramblingSubsystem;
	uint8_t messageAuthenticationControl;
	uint8_t scramblingInitialCounterValue;
};

}