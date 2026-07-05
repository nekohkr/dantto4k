#pragma once
#include "mpuProcessorBase.h"

namespace MmtTlv {

class MpuApplicationProcessor : public MpuProcessorTemplate<AssetType::aapp> {
public:
	std::optional<MfuData> process(MmtStream& mmtStream, std::span<const uint8_t> data, FragmentationIndicator fragmentationIndicator) override;

};

}
