#pragma once
#include <vector>
#include "stream.h"

namespace MmtTlv {

class DataUnit {
public:
	bool unpack(Common::Stream& stream, bool timedFlag, bool aggregateFlag);

	uint16_t dataUnitLength;
	uint32_t movieFragmentSequenceNumber;
	uint32_t sampleNumber;
	uint32_t offset;
	uint8_t priority;
	uint8_t dependencyCounter;
	uint32_t itemId;
	std::vector<uint8_t> data;
};

}