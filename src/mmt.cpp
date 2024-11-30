#include "mmt.h"
#include "extensionHeaderScrambling.h"
#include "acascard.h"
#include <openssl/evp.h>

namespace MmtTlv {

void Mmt::unpack(Common::ReadStream& stream)
{
	try {
		uint8_t uint8 = stream.get8U();
		version = (uint8 & 0b11000000) >> 6;
		packetCounterFlag = (uint8 & 0b00100000) >> 5;
		fecType = (uint8 & 0b00011000) >> 3;
		reserved1 = (uint8 & 0b00000100) >> 2;
		extensionHeaderFlag = (uint8 & 0b00000010) >> 1;
		rapFlag = uint8 & 0b00000001;

		uint8 = stream.get8U();
		reserved2 = (uint8 & 0b11000000) >> 6;
		payloadType = static_cast<PayloadType>(uint8 & 0b00111111);

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
			stream.read(extensionHeaderField.data(), extensionHeaderLength);

			if (extensionHeaderField.size() >= 5) {
				uint16_t e = Common::swapEndian16(*(uint16_t*)extensionHeaderField.data());
				if ((e & 0x7FFF) == 0x0001) {
					Common::ReadStream nstream(extensionHeaderField);
					nstream.skip(4);

					ExtensionHeaderScrambling s;
					if (s.unpack(nstream, extensionHeaderType, extensionHeaderLength)) {
						extensionHeaderScrambling = s;
					}
				}
			}
		}
		else {
			extensionHeaderScrambling = std::nullopt;
		}

		payload.resize(stream.leftBytes());
		stream.read(payload.data(), stream.leftBytes());
	}
	catch (const std::out_of_range&) {
		return;
	}
}

bool Mmt::decryptPayload(Acas::DecryptedEcm* decryptedEcm)
{
	if (packetCounterFlag) {
		//throw std::runtime_error("Has packet counter flag");
	}

	if (!extensionHeaderScrambling) {
		return false;
	}

	std::vector<uint8_t> key(16);
	if (extensionHeaderScrambling->encryptionFlag == EncryptionFlag::ODD) {
		memcpy(key.data(), decryptedEcm->odd, 16);
	}
	else if (extensionHeaderScrambling->encryptionFlag == EncryptionFlag::EVEN) {
		memcpy(key.data(), decryptedEcm->even, 16);
	}
	else {
		//throw std::runtime_error("Encryption flag reserved");
	}

	if (extensionHeaderScrambling->messageAuthenticationControl > 0) {
		//throw std::runtime_error("SICV not implemented");
	}

	if (extensionHeaderScrambling->messageAuthenticationControl > 0) {
		//throw std::runtime_error("MAC not implemented");
	}

	std::vector<uint8_t> iv;
	iv.assign(16, 0);

	uint16_t swappedPacketId = Common::swapEndian16(packetId);
	uint32_t swappedPacketSequenceNumber = Common::swapEndian32(packetSequenceNumber);
	memcpy(iv.data(), &swappedPacketId, 2);
	memcpy(iv.data() + 2, &swappedPacketSequenceNumber, 4);

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	EVP_EncryptInit_ex(ctx, EVP_aes_128_ctr(), nullptr, key.data(), iv.data());

	int outlen;
	EVP_EncryptUpdate(ctx, payload.data() + 8, &outlen, payload.data() + 8, static_cast<int>(payload.size() - 8));
	EVP_EncryptFinal_ex(ctx, payload.data() + 8 + outlen, &outlen);
	EVP_CIPHER_CTX_free(ctx);

	return true;
}

}