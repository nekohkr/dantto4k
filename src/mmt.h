#pragma once
#include <cstdint>
#include <optional>
#include "stream.h"
#include "extensionHeaderScrambling.h"

namespace MmtTlv {

enum class PayloadType
{
	Mpu = 0x00,
	Undefined = 0x01,
	ContainsOneOrMoreControlMessage = 0x02
};

namespace MmtPacketId {

constexpr uint16_t PaMessage = 0x0000;
constexpr uint16_t CaMessage = 0x0001;
constexpr uint16_t MhEit = 0x8000;
constexpr uint16_t MhAit = 0x8001;
constexpr uint16_t MhBit = 0x8002;
constexpr uint16_t MhSdtt = 0x8003;
constexpr uint16_t MhSdt = 0x8004;
constexpr uint16_t MhTot = 0x8005;
constexpr uint16_t MhCdt = 0x8006;
constexpr uint16_t DataTransmissionMessage = 0x8007;
constexpr uint16_t MhDit = 0x8008;
constexpr uint16_t MhSit = 0x8009;

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