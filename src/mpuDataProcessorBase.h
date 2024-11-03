#pragma once
#include <vector>
#include <optional>
#include "mpuStream.h"

namespace MmtTlv {

constexpr int32_t makeAssetType(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
	return (a << 24) | (b << 16) | (c << 8) | d;
}

struct MpuData {
	std::vector<uint8_t> data;
	int64_t pts;
	int64_t dts;
	int streamIndex;
	int flags;
};

class MpuDataProcessorBase {
public:
	virtual ~MpuDataProcessorBase() = default;
	virtual std::optional<MpuData> process(const std::shared_ptr<MpuStream>& mpuStream, const std::vector<uint8_t>& mpuData) { return std::nullopt; }

};

template<uint32_t assetType>
class MpuDataProcessorTemplate : public MpuDataProcessorBase {
public:
	virtual ~MpuDataProcessorTemplate() = default;

	static constexpr uint32_t kAssetType = assetType;

};

}