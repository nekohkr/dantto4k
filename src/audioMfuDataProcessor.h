#pragma once
#include "mfuDataProcessorBase.h"

namespace MmtTlv {

class AudioMfuDataProcessor : public MfuDataProcessorTemplate<makeAssetType('m', 'p', '4', 'a')> {
public:
	std::optional<MfuData> process(const std::shared_ptr<MmtStream>& mmtStream, const std::vector<uint8_t>& data);

private:
	std::vector<uint8_t> pendingData;
};

}