#pragma once
#include <vector>
#include "mpuExtendedTimestampDescriptor.h"
#include "mpuTimestampDescriptor.h"

class MpuDataProcessorBase;
class MmtStream {
public:
	std::pair<int64_t, int64_t> calcPtsDts() const;
	void incrementAuIndex();
	uint32_t getAuIndex() const;
	//enum AVMediaType codecType;
	//enum AVCodecID codecId = AV_CODEC_ID_NONE;
	//uint32_t codecTag = 0;

	uint32_t assetType;
	uint32_t lastMpuSequenceNumber = 0;
	uint32_t auIndex = 0;
	uint32_t streamIndex = 0;
	uint16_t pid = 0;

	int flags = 0;

	std::vector<MpuTimestampDescriptor::Entry> mpuTimestamps;
	std::vector<MpuExtendedTimestampDescriptor::Entry> mpuExtendedTimestamps;

	std::shared_ptr<MpuDataProcessorBase> mpuDataProcessor;

	struct TimeBase{
		int num;
		int den;
	};

	struct TimeBase timeBase;

private:
	std::pair<const MpuTimestampDescriptor::Entry, const MpuExtendedTimestampDescriptor::Entry> getCurrentTimestamp() const;
};

