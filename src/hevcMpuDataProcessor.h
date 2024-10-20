#pragma once
#include "mpuDataProcessorBase.h"

class HevcMpuDataProcessor : public MpuDataProcessorTemplate<makeAssetType('h', 'e', 'v', '1')> {
public:
	std::optional<MpuData> process(const std::shared_ptr<MmtStream>& mmtStream, const std::vector<uint8_t>& mpuData);

private:
	std::vector<uint8_t> pendingData;
};