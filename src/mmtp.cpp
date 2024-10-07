#include "mmtp.h"
#include "stream.h"
#include "acascard.h"
#include "mpuTimestampDescriptor.h"
#include "mpuExtendedTimestampDescriptor.h"
#include <openssl/evp.h>

void Mmtp::unpack(Stream& stream)
{
	if (stream.leftBytes() < 1+1+2+4+4) {
		return;
	}

	uint8_t uint8 = stream.get8U();
	version =				(uint8 & 0b11000000) >> 6;
	packetCounterFlag =		(uint8 & 0b00100000) >> 5;
	fecType =				(uint8 & 0b00011000) >> 3;
	reserved1 =				(uint8 & 0b00000100) >> 2;
	extensionHeaderFlag =	(uint8 & 0b00000010) >> 1;
	rapFlag =				 uint8 & 0b00000001;

	uint8 = stream.get8U();
	reserved2 = (uint8 & 0b11000000) >> 6;
	payloadType = uint8 & 0b00111111;

	packetId = stream.getBe16U();
	deliveryTimestamp = stream.getBe32U();
	packetSequenceNumber = stream.getBe32U();

	if (packetCounterFlag) {
		if (stream.leftBytes() < 4) {
			return;
		}
		packetCounter = stream.getBe32U();
	}


	if (extensionHeaderFlag) {
		if (stream.leftBytes() < 4) {
			return;
		}
		extensionHeaderType = stream.getBe16U();
		extensionHeaderLength = stream.getBe16U();

		if (stream.leftBytes() < extensionHeaderLength) {
			return;
		}
		extensionHeaderField.resize(extensionHeaderLength);
		stream.read((char*)extensionHeaderField.data(), extensionHeaderLength);


		if (extensionHeaderField.size() >= 5) {
			uint16_t e = swapEndian16(*(uint16_t*)extensionHeaderField.data());
			if ((e & 0x7FFF) == 0x0001) {
				Stream nstream(extensionHeaderField);
				nstream.skip(4);
				if (extensionHeaderScrambling.unpack(extensionHeaderType, extensionHeaderLength, nstream)) {
					hasExtensionHeaderScrambling = true;
				}
			}
		}
	}
	else {
		hasExtensionHeaderScrambling = false;
		extensionHeaderScrambling.encryptionFlag = ENCRYPTION_FLAG::UNSCRAMBLED;
		extensionHeaderScrambling.messageAuthenticationControl = 0;
		extensionHeaderScrambling.scramblingInitialCounterValue = 0;
		extensionHeaderScrambling.scramblingSubsystem = 0;
	}

	payload.resize(stream.leftBytes());
	stream.read((char*)payload.data(), stream.leftBytes());
}

void Mmtp::pack(Stream& stream) const
{
	uint8_t uint8;
	uint8 = (version & 0b00000011) << 6;
	uint8 |= (packetCounterFlag & 0b00000001) << 5;
	uint8 |= (fecType & 0b00000011) << 3;
	uint8 |= (reserved1 & 0b00000001) << 2;
	uint8 |= (extensionHeaderFlag & 0b00000001) << 1;
	uint8 |= (rapFlag & 0b00000001);
	stream.put8U(uint8);

	uint8 = (uint8 & 0b11000000) << 6;
	uint8 |= uint8 & 0b00111111;
	stream.put8U(uint8);

	stream.putBe16U(packetId);
	stream.putBe32U(deliveryTimestamp);
	stream.putBe32U(packetSequenceNumber);
}

bool Mmtp::decryptPayload(DecryptedEcm* decryptedEcm)
{
	if (packetCounterFlag) {
		throw std::runtime_error("Has packet counter flag");
	}

	std::vector<uint8_t> key(16);
	if (extensionHeaderScrambling.encryptionFlag == ENCRYPTION_FLAG::ODD) {
		memcpy(key.data(), decryptedEcm->odd, 16);
	}
	else if (extensionHeaderScrambling.encryptionFlag == ENCRYPTION_FLAG::EVEN) {
		memcpy(key.data(), decryptedEcm->even, 16);
	}
	else {
		throw std::runtime_error("Encryption flag reserved");
	}

	if (extensionHeaderScrambling.messageAuthenticationControl > 0) {
		throw std::runtime_error("SICV not implemented");
	}

	if (extensionHeaderScrambling.messageAuthenticationControl > 0) {
		throw std::runtime_error("MAC not implemented");
	}

	std::vector<uint8_t> iv;
	iv.assign(16, 0);

	uint16_t swappedPacketId = swapEndian16(packetId);
	uint32_t swappedPacketSequenceNumber = swapEndian32(packetSequenceNumber);
	memcpy(iv.data(), &swappedPacketId, 2);
	memcpy(iv.data() + 2, &swappedPacketSequenceNumber, 4);

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	EVP_EncryptInit_ex(ctx, EVP_aes_128_ctr(), nullptr, key.data(), iv.data());

	int outlen;
	EVP_EncryptUpdate(ctx, payload.data() + 8, &outlen, payload.data() + 8, payload.size() - 8);
	EVP_EncryptFinal_ex(ctx, payload.data() + 8 + outlen, &outlen);
	EVP_CIPHER_CTX_free(ctx);

	return true;
}

bool ExtensionHeaderScrambling::unpack(uint16_t extensionHeaderType, uint16_t extensionHeaderLength, Stream& stream)
{
	if (stream.leftBytes() < 1) {
		return false;
	}

	uint8_t uint8 = stream.get8U();
	encryptionFlag = (ENCRYPTION_FLAG)((uint8 & 0b00011000) >> 3);
	scramblingSubsystem = (uint8 & 0b00000100) >> 2;
	messageAuthenticationControl = (uint8 & 0b00000010) >> 1;
	scramblingInitialCounterValue = uint8 & 0b00000001;

	return true;
}

bool Mpu::unpack(Stream& stream)
{
	if (stream.leftBytes() < 2 + 1 + 1 + 4)
		return false;

	payloadLength = stream.getBe16U();
	if (payloadLength != stream.leftBytes())
		return false;

	uint8_t byte = stream.get8U();
	fragmentType = (MPU_FRAGMENT_TYPE)(byte >> 4);
	timedFlag = (byte >> 3) & 1;
	fragmentationIndicator = (byte >> 1) & 0b11;
	aggregateFlag = byte & 1;

	fragmentCounter = stream.get8U();
	mpuSequenceNumber = stream.getBe32U();


	payload.resize(payloadLength - 6);
	stream.read((char*)payload.data(), payloadLength - 6);

	return true;
}

bool DataUnit::unpack(Stream& stream, bool timedFlag, bool aggregateFlag)
{
	if (timedFlag) {
		if (aggregateFlag == 0) {
			if (stream.leftBytes() < 4 + 4 + 4 + 1 + 1) {
				return false;
			}

			movieFragmentSequenceNumber = stream.getBe32U();
			sampleNumber = stream.getBe32U();
			offset = stream.getBe32U();
			priority = stream.get8U();
			dependencyCounter = stream.get8U();

			data.resize(stream.leftBytes());
			stream.read((char*)data.data(), stream.leftBytes());
		}
		else {
			if (stream.leftBytes() < 2+1+4+4+4+1+1) {
				return false;
			}

			dataUnitLength = stream.getBe16U();

			size_t leftBytes = stream.leftBytes();
			dataUnitLength = min(dataUnitLength, stream.leftBytes());

			movieFragmentSequenceNumber = stream.getBe32U();
			sampleNumber = stream.getBe32U();
			offset = stream.getBe32U();
			priority = stream.get8U();
			dependencyCounter = stream.get8U();

			if (dataUnitLength < 4 * 3 + 2) {
				return false;
			}
			data.resize(dataUnitLength - 4 * 3 - 2);
			stream.read((char*)data.data(), dataUnitLength - 4 * 3 - 2);
		}
	} else {
		if (aggregateFlag == 0) {
			if (stream.leftBytes() < 4) {
				return false;
			}

			itemId = stream.getBe32U();

			data.resize(stream.leftBytes());
			stream.read((char*)data.data(), stream.leftBytes());
		} else {
			if (stream.leftBytes() < 2) {
				return false;
			}

			dataUnitLength = stream.getBe16U();

			data.resize(dataUnitLength);
			stream.read((char*)data.data(), dataUnitLength);
		}
	}
	return true;
}

bool PaMessage::unpack(Stream& stream)
{
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
	stream.read((char*)table.data(), stream.leftBytes());

	return false;
}


bool SignalingMessage::unpack(Stream& stream)
{
	if (stream.leftBytes() < 1 + 1) {
		return false;
	}

	uint8_t uint8 = stream.get8U();
	fragmentationIndicator = (uint8 & 0b11000000) >> 6;
	reserved = (uint8 & 0b00111100) >> 2;
	lengthExtensionFlag = (uint8 & 0x00000010) >> 2;
	aggregationFlag = uint8 & 1;

	fragmentCounter = stream.get8U();

	payload.resize(stream.leftBytes());
	stream.read((char*)payload.data(), stream.leftBytes());

	return true;
}

bool M2SectionMessage::unpack(Stream& stream)
{
	if (stream.leftBytes() < 2 + 1 + 2) {
		return false;
	}

	messageId = stream.getBe16U();
	version = stream.get8U();
	length = stream.getBe16U();

	return true;
}

bool M2ShortSectionMessage::unpack(Stream& stream)
{
	if (stream.leftBytes() < 2 + 1 + 2) {
		return false;
	}

	messageId = stream.getBe16U();
	version = stream.get8U();
	length = stream.getBe16U();

	return true;
}
