#pragma once
#include "mfuDataProcessorBase.h"

namespace MmtTlv {

class ApplicationMfuDataProcessor : public MfuDataProcessorTemplate<AssetType::aapp> {
public:
	std::optional<MfuData> process(const std::shared_ptr<MmtStream>& mmtStream, const std::vector<uint8_t>& data);

};

}