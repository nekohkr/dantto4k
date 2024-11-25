#pragma once
#include <cstdint>
#include "stream.h"

namespace MmtTlv {

class M2SectionMessage {
public:
	bool unpack(Common::ReadStream& stream);

	uint16_t messageId;
	uint8_t version;
	uint16_t length;
};

}