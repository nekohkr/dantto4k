#pragma once
#include <vector>
#include "stream.h"
#include "mmtFragment.h"

namespace MmtTlv {

class Mpu {
public:
	bool unpack(Common::Stream& stream);

	uint16_t payloadLength;
	FragmentType fragmentType;
	bool timedFlag;
	FragmentationIndicator fragmentationIndicator;
	bool aggregateFlag;
	uint8_t fragmentCounter;
	uint32_t mpuSequenceNumber;
	std::vector<uint8_t> payload;
};

}
