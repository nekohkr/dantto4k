#pragma once
#include <cstdint>
#include "stream.h"

namespace MmtTlv {

class M2ShortSectionMessage {
public:
	bool unpack(Common::Stream& stream);

	uint16_t messageId;
	uint8_t version;
	uint16_t length;
};

}