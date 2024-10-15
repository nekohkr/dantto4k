#pragma once
#include <vector>
#include <map>
#include <list>
#include "tlv.h"
#include "mmtGeneralLocationInfo.h"

enum MMT_TABLE_ID {
	MPT = 0x20,
	PLT = 0x80,
	ECM = 0x82,
	MH_EIT = 0x8B,
	MH_SDT = 0x9F,
	MH_TOT = 0xA1,
	MH_CDT = 0xA2,
};

enum TLV_TABLE_ID {
	TLV_NIT = 0x40,
};

enum PAYLOAD_TYPE
{
	MPU = 0x00,
	UNDEFINED = 0x01,
	CONTAINS_ONE_OR_MORE_CONTROL_MESSAGE = 0x02
};

enum class MPU_FRAGMENT_TYPE : uint8_t
{
	MPU_METADATA = 0x00,
	MOVICE_FRAGMENT_METADATA = 0x01,
	MPU = 0x02
};

enum class ENCRYPTION_FLAG : uint8_t
{
	UNSCRAMBLED = 0x00,
	RESERVED = 0x01,
	EVEN = 0x02,
	ODD = 0x03,
};

class Stream;
struct DecryptedEcm;
class MmtTlvDemuxer;





class PaMessage {
public:
	bool unpack(Stream& stream);

	uint16_t messageId;
	uint8_t version;
	uint32_t length;
	uint8_t numberOfTables;
	std::vector<uint8_t> table;
};

class M2SectionMessage {
public:
	bool unpack(Stream& stream);

	uint16_t messageId;
	uint8_t version;
	uint16_t length;
};

class M2ShortSectionMessage {
public:
	bool unpack(Stream& stream);

	uint16_t messageId;
	uint8_t version;
	uint16_t length;
};

class DataUnit {
public:
	bool unpack(Stream& stream, bool timedFlag, bool aggregateFlag);
	uint16_t dataUnitLength;
	uint32_t movieFragmentSequenceNumber;
	uint32_t sampleNumber;
	uint32_t offset;
	uint8_t priority;
	uint8_t dependencyCounter;
	uint32_t itemId;
	std::vector<uint8_t> data;
};

class Mpu {
public:
	bool unpack(Stream& stream);

	uint16_t payloadLength;

	MPU_FRAGMENT_TYPE fragmentType;
	bool timedFlag;
	uint8_t fragmentationIndicator;
	bool aggregateFlag;
	uint8_t fragmentCounter;
	uint32_t mpuSequenceNumber;

	std::vector<uint8_t> payload;
};

class SignalingMessage {
public:
	bool unpack(Stream& stream);

	uint8_t fragmentationIndicator;
	uint8_t reserved;
	bool lengthExtensionFlag;
	bool aggregationFlag;
	uint8_t fragmentCounter;

	std::vector<uint8_t> payload;
};

class ExtensionHeaderScrambling {
public:
	bool unpack(uint16_t extensionHeaderType, uint16_t extensionHeaderLength, Stream& stream);
	//void pack(Stream& stream) const;
	ENCRYPTION_FLAG encryptionFlag;
	uint8_t scramblingSubsystem;
	uint8_t messageAuthenticationControl;
	uint8_t scramblingInitialCounterValue;
};

class Mmtp {
public:
	Mmtp() {}
	void unpack(Stream& stream);
	void unpackPayload();
	void pack(Stream& stream) const;

	bool decryptPayload(DecryptedEcm* decryptedEcm);

	uint8_t version;
	bool packetCounterFlag;
	uint8_t fecType;
	bool reserved1;
	bool extensionHeaderFlag;
	bool rapFlag;
	uint8_t reserved2;
	uint8_t payloadType;
	uint16_t packetId;
	uint32_t deliveryTimestamp;
	uint32_t packetSequenceNumber;
	uint32_t packetCounter;
	uint16_t extensionHeaderType;
	uint16_t extensionHeaderLength;
	std::vector<uint8_t> extensionHeaderField;
	std::vector<uint8_t> payload;

	bool hasExtensionHeaderScrambling;
	ExtensionHeaderScrambling extensionHeaderScrambling;

};