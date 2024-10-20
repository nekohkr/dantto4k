#pragma once
#include "mpuDataProcessorBase.h"

class TtmlMpuDataProcessor : public MpuDataProcessorTemplate<makeAssetType('s', 't', 'p', 'p')> {
public:
	std::optional<MpuData> process(const std::shared_ptr<MmtStream>& mmtStream, const std::vector<uint8_t>& mpuData);

private:
	std::vector<uint8_t> pendingData;
};