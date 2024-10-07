#pragma once
#include "mmtDescriptor.h"

class MpuTimestamp {
public:
	uint32_t mpuSequenceNumber;
	uint64_t mpuPresentationTime;
};

class NTPTimestamp {
public:
	bool unpack(Stream& stream);
	uint64_t ntp;
};

class MpuTimestampDescriptor : public MmtDescriptor {
public:
	bool unpack(Stream& stream);

	std::vector<MpuTimestamp> mpuTimestamps;
};