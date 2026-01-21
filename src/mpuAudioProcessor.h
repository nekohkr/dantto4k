#pragma once
#include "mpuProcessorBase.h"

namespace MmtTlv {

class MpuAudioProcessor : public MpuProcessorTemplate<AssetType::mp4a> {
public:
	std::optional<MfuData> process(MmtStream& mmtStream, const std::vector<uint8_t>& data) override;

private:
	std::vector<uint8_t> pendingData;
};

}