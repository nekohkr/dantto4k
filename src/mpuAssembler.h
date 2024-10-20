#pragma once
#include <vector>
#include <iostream>

enum FragmentationIndicator {
	NOT_FRAGMENTED = 0b00,
	FIRST_FRAGMENT = 0b01,
	MIDDLE_FRAGMENT = 0b10,
	LAST_FRAGMENT = 0b11,
};

enum class MPU_ASSEMBLER_STATE
{
	INIT,
	NOT_STARTED,
	IN_FRAGMENT,
	SKIP,
};

class MpuAssembler {
public:
	bool assemble(const std::vector<uint8_t>& fragment, uint8_t fragmentationIndicator, uint32_t packetSequenceNumber);
	void checkState(uint32_t packetSequenceNumber);
	void clear();

	MPU_ASSEMBLER_STATE state = MPU_ASSEMBLER_STATE::INIT;
	std::vector<uint8_t> data;
	uint32_t last_seq = 0;
};

