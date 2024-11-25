#pragma once
#include <vector>
#include "stream.h"

namespace MmtTlv {

class PaMessage {
public:
	bool unpack(Common::ReadStream& stream);

	uint16_t messageId;
	uint8_t version;
	uint32_t length;
	uint8_t numberOfTables;
	std::vector<uint8_t> table;
};

}