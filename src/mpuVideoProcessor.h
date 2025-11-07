#pragma once
#include "mpuProcessorBase.h"

namespace MmtTlv {

class MpuVideoProcessor : public MpuProcessorTemplate<AssetType::hev1> {
public:
	std::optional<MpuData> process(const std::shared_ptr<MmtStream>& mmtStream, const std::vector<uint8_t>& data) override;

private:
	void appendPendingData(Common::ReadStream& stream, int size);

	std::vector<uint8_t> pendingData;
	int sliceSegmentCount = 0;

};

}