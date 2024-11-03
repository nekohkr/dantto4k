#pragma once
#include "mpuDataProcessorBase.h"

namespace MmtTlv {

class VideoMpuDataProcessor : public MpuDataProcessorTemplate<makeAssetType('h', 'e', 'v', '1')> {
public:
	std::optional<MpuData> process(const std::shared_ptr<MpuStream>& mpuStream, const std::vector<uint8_t>& mpuData);

private:
	std::vector<uint8_t> pendingData;
};

}