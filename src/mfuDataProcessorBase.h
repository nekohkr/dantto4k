#pragma once
#include <vector>
#include <optional>
#include "mmtStream.h"

namespace MmtTlv {

constexpr int32_t makeAssetType(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
	return (a << 24) | (b << 16) | (c << 8) | d;
}

namespace AssetType {
	constexpr int32_t hev1 = makeAssetType('h', 'e', 'v', '1');
	constexpr int32_t mp4a = makeAssetType('m', 'p', '4', 'a');
	constexpr int32_t stpp = makeAssetType('s', 't', 'p', 'p');
	constexpr int32_t aapp = makeAssetType('a', 'a', 'p', 'p');
}

struct MfuData {
	std::vector<uint8_t> data;
	int64_t pts;
	int64_t dts;
	int streamIndex;
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