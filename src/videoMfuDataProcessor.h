#pragma once
#include "mfuDataProcessorBase.h"

namespace MmtTlv {

class VideoMfuDataProcessor : public MfuDataProcessorTemplate<AssetType::hev1> {
public:
	std::optional<MfuData> process(const std::shared_ptr<MmtStream>& mmtStream, const std::vector<uint8_t>& data);

private:
	void appendPendingData(Common::ReadStream& stream, int size);

	std::vector<uint8_t> pendingData;
	int sliceSegmentCount = 0;

};

}