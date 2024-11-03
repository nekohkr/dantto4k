#pragma once
#include "mpuDataProcessorBase.h"

namespace MmtTlv {

class AudioMpuDataProcessor : public MpuDataProcessorTemplate<makeAssetType('m', 'p', '4', 'a')> {
public:
	std::optional<MpuData> process(const std::shared_ptr<MpuStream>& mpuStream, const std::vector<uint8_t>& mpuData);

private:
	std::vector<uint8_t> pendingData;
};

}