#pragma once
#include "mpuProcessorBase.h"

namespace MmtTlv {

class MpuAudioProcessor : public MpuProcessorTemplate<AssetType::mp4a> {
public:
	std::optional<MfuData> process(MmtStream& mmtStream, const std::vector<uint8_t>& data, FragmentationIndicator fragmentationIndicator) override;
	void clear() override;

private:
	size_t aacFrameSize{0};
	size_t aacFragmentOffset{0};
	std::vector<uint8_t> aacFragment;

};

}