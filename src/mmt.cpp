#include "mmt.h"
#include "extensionHeaderScrambling.h"

namespace MmtTlv {

bool Mmt::unpack(Common::ReadStream& stream) {
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
				return false;
			}
			packetCounter = stream.getBe32U();
		}

		if (extensionHeaderFlag) {
			if (stream.leftBytes() < 4) {
				return false;
			}
			extensionHeaderType = stream.getBe16U();
			extensionHeaderLength = stream.getBe16U();

			if (stream.leftBytes() < extensionHeaderLength) {
				return false;
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
		return false;
	}

	return true;
}

}