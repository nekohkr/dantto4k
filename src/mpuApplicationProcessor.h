#pragma once
#include "mpuProcessorBase.h"

namespace MmtTlv {

class MpuApplicationProcessor : public MpuProcessorTemplate<AssetType::aapp> {
public:
	std::optional<MpuData> process(const std::shared_ptr<MmtStream>& mmtStream, const std::vector<uint8_t>& data) override;

};

}