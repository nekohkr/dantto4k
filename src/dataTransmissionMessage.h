#pragma once
#include <cstdint>
#include "stream.h"

namespace MmtTlv {

class DataTransmissionMessage {
public:
	bool unpack(Common::ReadStream& stream);

	uint16_t messageId;
	uint8_t version;
	uint32_t length;
};

}