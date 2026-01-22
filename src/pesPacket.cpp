#include "pesPacket.h"
#include "stream.h"

namespace {

void writePts(MmtTlv::Common::WriteStream &stream, int fourbits, int64_t pts) {
	int val;

	val  = fourbits << 4 | (((pts >> 30) & 0x07) << 1) | 1;
	stream.put8U(val);
	val  = (((pts >> 15) & 0x7fff) << 1) | 1;
	stream.put8U(val >> 8);
	stream.put8U(val);
	val  = (((pts) & 0x7fff) << 1) | 1;
	stream.put8U(val >> 8);
	stream.put8U(val);
}

}

bool PESPacket::pack(std::vector<uint8_t>& output) {
	MmtTlv::Common::WriteStream stream;

	// packet_start_code_prefix
	stream.write({0x00, 0x00, 0x01});
	stream.put8U(streamId);

	uint8_t flags = 0;
	uint8_t headerLength = 0;

	if (pts != NOPTS_VALUE) {
		headerLength += 5;
		flags |= 0b10000000;
	}
	
	if (dts != NOPTS_VALUE && pts != NOPTS_VALUE && dts != pts) {
		headerLength += 5;
		flags |= 0b01000000;
	}

	if (privateData) {
		flags |= 0b00000001;
		headerLength += 1 + static_cast<uint8_t>(privateData->size());
	}

	if (stuffingByteLength) {
		headerLength += stuffingByteLength;
	}

	size_t length = 0;
	if (payloadLength > 0) {
		length = payloadLength + headerLength;
        if (streamId != 0xBF) {
            length += 3;
        }
	}

	if (length > 0xffff) {
		length = 0;
	}

	stream.putBe16U(static_cast<uint16_t>(length));
	if (streamId != 0xBF) {
		stream.put8U(2 << 6 /* reserved */ | dataAlignmentIndicator << 2);
		stream.put8U(flags);
		stream.put8U(headerLength);
	}

	if (pts != NOPTS_VALUE) {
		writePts(stream, flags >> 6, pts);
	}
	if (dts != NOPTS_VALUE && pts != NOPTS_VALUE && dts != pts) {
		writePts(stream, 1, dts);
	}

	if (privateData) {
		stream.put8U(0b10001110);
		stream.write(std::span<const uint8_t>{privateData->data(), privateData->size()});
	}

	if (stuffingByteLength) {
		for (int i = 0; i < stuffingByteLength; i++) {
			stream.put8U(0xFF);
		}
	}

	if (payload) {
		stream.write(std::span<const uint8_t>{payload->data(), payload->size()});
	}

	output = stream.getData();
	return true;
}