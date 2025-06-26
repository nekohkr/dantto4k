#pragma once
#include <cstdint>
#include <optional>
#include "stream.h"
#include "extensionHeaderScrambling.h"

namespace MmtTlv {

namespace Acas {
	struct DecryptedEcm;
}

enum class PayloadType
{
	Mpu = 0x00,
	Undefined = 0x01,
	ContainsOneOrMoreControlMessage = 0x02
};

class Mmt {
public:
	bool unpack(Common::ReadStream& stream);
	bool decryptPayload(const std::array<uint8_t, 16>& key);

public:
	uint8_t version;
	bool packetCounterFlag;
	uint8_t fecType;
	bool reserved1;
	bool extensionHeaderFlag;
	bool rapFlag;
	uint8_t reserved2;
	PayloadType payloadType;
	uint16_t packetId;
	uint32_t deliveryTimestamp;
	uint32_t packetSequenceNumber;
	uint32_t packetCounter;
	uint16_t extensionHeaderType;
	uint16_t extensionHeaderLength;
	std::vector<uint8_t> extensionHeaderField;
	std::vector<uint8_t> payload;

	std::optional<ExtensionHeaderScrambling> extensionHeaderScrambling;

};

}