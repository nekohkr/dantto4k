#pragma once
#include <vector>
#include <optional>
#include "mmtStream.h"

namespace MmtTlv {

constexpr int32_t makeAssetType(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
	return (a << 24) | (b << 16) | (c << 8) | d;
}

struct MfuData {
	std::vector<uint8_t> data;
	int64_t pts;
	int64_t dts;
	int streamIndex;
	int flags;
};

class MfuDataProcessorBase {
public:
	virtual ~MfuDataProcessorBase() = default;
	virtual std::optional<MfuData> process(const std::shared_ptr<MmtStream>& mmtStream, const std::vector<uint8_t>& data) { return std::nullopt; }

};

template<uint32_t assetType>
class MfuDataProcessorTemplate : public MfuDataProcessorBase {
public:
	virtual ~MfuDataProcessorTemplate() = default;

	static constexpr uint32_t kAssetType = assetType;

};

}