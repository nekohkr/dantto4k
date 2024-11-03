#include "audioMpuDataProcessor.h"

namespace MmtTlv {

std::optional<MpuData> AudioMpuDataProcessor::process(const std::shared_ptr<MpuStream>& mpuStream, const std::vector<uint8_t>& data)
{
    Common::Stream stream(data);
    uint32_t size = stream.leftBytes();

    std::pair<int64_t, int64_t> ptsDts;
    try {
        ptsDts = mpuStream->calcPtsDts();
    }
    catch (const std::out_of_range&) {
        return std::nullopt;
    }

    mpuStream->incrementAuIndex();

    MpuData mpuData;
    mpuData.data.resize(size + 3);
    mpuData.data[0] = 0x56;
    mpuData.data[1] = ((size >> 8) & 0x1F) | 0xE0;
    mpuData.data[2] = size & 0xFF;
    stream.read(mpuData.data.data() + 3, size);

    mpuData.pts = ptsDts.first;
    mpuData.dts = ptsDts.second;
    mpuData.streamIndex = mpuStream->streamIndex;
    mpuData.flags = mpuStream->flags;

    mpuStream->flags = 0;

	return mpuData;
}

}