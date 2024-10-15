#pragma once
#include "mmtDescriptor.h"

class MpuTimestampDescriptor
	: public MmtDescriptor<0x0001> {
public:
	bool unpack(Stream& stream) override;

	class Entry {
	public:
		uint32_t mpuSequenceNumber;
		uint64_t mpuPresentationTime;
	};

	std::vector<Entry> entries;
};