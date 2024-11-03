#pragma once
#include "mpuDataProcessorBase.h"

namespace MmtTlv {

class SubtitleMpuDataProcessor : public MpuDataProcessorTemplate<makeAssetType('s', 't', 'p', 'p')> {
public:
	std::optional<MpuData> process(const std::shared_ptr<MpuStream>& mpuStream, const std::vector<uint8_t>& mpuData);

private:
	std::vector<uint8_t> pendingData;
};

}