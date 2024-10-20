#include "aacMpuDataProcessor.h"

std::optional<MpuData> AacMpuDataProcessor::process(const std::shared_ptr<MmtStream>& mmtStream, const std::vector<uint8_t>& data)
{
    Stream stream(data);
    uint32_t size = stream.leftBytes();

    std::pair<int64_t, int64_t> ptsDts;
    try {
        ptsDts = mmtStream->calcPtsDts();
    }
    catch (const std::out_of_range&) {
        return std::nullopt;
    }

    mmtStream->incrementAuIndex();

    MpuData mpuData;
    mpuData.data.resize(size + 3);
    mpuData.data[0] = 0x56;
    mpuData.data[1] = ((size >> 8) & 0x1F) | 0xE0;
    mpuData.data[2] = size & 0xFF;
    stream.read(mpuData.data.data() + 3, size);

    mpuData.pts = ptsDts.first;
    mpuData.dts = ptsDts.second;
    mpuData.streamIndex = mmtStream->streamIndex;
    mpuData.flags = mmtStream->flags;

    mmtStream->flags = 0;

	return mpuData;
}