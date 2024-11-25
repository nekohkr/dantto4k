#pragma once
#include <vector>
#include "stream.h"
#include "mmtFragment.h"

namespace MmtTlv {

class SignalingMessage {
public:
	bool unpack(Common::ReadStream& stream);

	FragmentationIndicator fragmentationIndicator;
	uint8_t reserved;
	bool lengthExtensionFlag;
	bool aggregationFlag;
	uint8_t fragmentCounter;

	std::vector<uint8_t> payload;
};

}