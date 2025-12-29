#pragma once
#include "mpuProcessorBase.h"

namespace MmtTlv {

class MpuVideoProcessor : public MpuProcessorTemplate<AssetType::hev1> {
public:
	std::optional<MfuData> process(const std::shared_ptr<MmtStream>& mmtStream, const std::vector<uint8_t>& data) override;
	void clear();

private:
	std::vector<uint8_t> buffer;
	int sliceSegmentCount = 0;
	size_t nalUnitSize = 0;
	int nalUnitType = 0;
    uint64_t pts = 0;
    uint64_t dts = 0;
	int streamIndex = 0;

};

}