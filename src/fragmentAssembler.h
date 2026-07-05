#pragma once
#include <vector>
#include <cstdint>
#include "mmtFragment.h"

namespace MmtTlv {

class FragmentValidator {
public:
	bool validate(FragmentationIndicator fragmentationIndicator, uint32_t packetSequenceNumber);
	void clear();

private:
	enum class State
	{
		Init,
		NotStarted,
		InFragment,
		Skip,
	};

	State state = State::Init;
};

class FragmentAssembler {
public:
	bool assemble(const std::vector<uint8_t>& fragment, FragmentationIndicator fragmentationIndicator, uint32_t packetSequenceNumber);
	void checkState(uint32_t packetSequenceNumber);
	void clear();

	enum class State
	{
		Init,
		NotStarted,
		InFragment,
		Skip,
	};

	State state = State::Init;
	std::vector<uint8_t> data;
	uint32_t last_seq = 0;
};

}