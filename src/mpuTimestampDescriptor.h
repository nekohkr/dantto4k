#pragma once
#include "mmtDescriptorBase.h"

namespace MmtTlv {

class MpuTimestampDescriptor
	: public MmtDescriptorTemplate<0x0001> {
public:
	bool unpack(Common::ReadStream& stream) override;

	class Entry {
	public:
		uint32_t mpuSequenceNumber;
		uint64_t mpuPresentationTime;
	};

	std::vector<Entry> entries;
};

}