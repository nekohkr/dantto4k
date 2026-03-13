#pragma once
#include "mpuProcessorBase.h"

namespace MmtTlv {

class MpuSubtitleProcessor : public MpuProcessorTemplate<AssetType::stpp> {
public:
	std::optional<MfuData> process(MmtStream& mmtStream, const std::vector<uint8_t>& data, FragmentationIndicator fragmentationIndicator) override;

private:
	std::vector<uint8_t> pendingData;
};

}