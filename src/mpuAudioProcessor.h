#pragma once
#include "mpuProcessorBase.h"

namespace MmtTlv {

class MpuAudioProcessor : public MpuProcessorTemplate<AssetType::mp4a> {
public:
	std::optional<MfuData> process(const std::shared_ptr<MmtStream>& mmtStream, const std::vector<uint8_t>& data) override;

private:
	std::vector<uint8_t> pendingData;
};

}