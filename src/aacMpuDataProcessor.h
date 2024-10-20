#pragma once
#include "mpuDataProcessorBase.h"

class AacMpuDataProcessor : public MpuDataProcessorTemplate<makeAssetType('m', 'p', '4', 'a')> {
public:
	std::optional<MpuData> process(const std::shared_ptr<MmtStream>& mmtStream, const std::vector<uint8_t>& mpuData);

private:
	std::vector<uint8_t> pendingData;
};