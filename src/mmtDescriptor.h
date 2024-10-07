#pragma once
#include "stream.h"

class MmtDescriptor {
public:
	virtual ~MmtDescriptor() = default;

	bool unpack(Stream& stream, bool is16BitLength = false);

	uint16_t descriptorTag;
	uint16_t descriptorLength;
};