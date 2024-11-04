#pragma once
#include <vector>
#include "mpuExtendedTimestampDescriptor.h"
#include "mpuTimestampDescriptor.h"

namespace MmtTlv {

class MfuDataProcessorBase;

class MmtStream {
public:
	std::pair<int64_t, int64_t> calcPtsDts() const;
	void incrementAuIndex();
	uint32_t getAuIndex() const;
	uint16_t getTsPid() const { return (componentTag == -1) ? 0x200 + streamIndex : 0x100 + componentTag; }

	uint32_t assetType;
	uint32_t lastMpuSequenceNumber = 0;
	uint32_t auIndex = 0;
	uint32_t streamIndex = 0;
	uint16_t pid = 0;
	int16_t componentTag = -1;

	int flags = 0;

	std::vector<MpuTimestampDescriptor::Entry> mpuTimestamps;
	std::vector<MpuExtendedTimestampDescriptor::Entry> mpuExtendedTimestamps;

	std::shared_ptr<MfuDataProcessorBase> mfuDataProcessor;

	struct TimeBase {
		int num;
		int den;
	};

	struct TimeBase timeBase;

private:
	std::pair<const MpuTimestampDescriptor::Entry, const MpuExtendedTimestampDescriptor::Entry> getCurrentTimestamp() const;
};

}
