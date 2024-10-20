#include "hevcMpuDataProcessor.h"
#include "stream.h"
#include "mmttlvdemuxer.h"

std::optional<MpuData> HevcMpuDataProcessor::process(const std::shared_ptr<MmtStream>& mmtStream, const std::vector<uint8_t>& data)
{
    Stream stream(data);
    if (stream.leftBytes() < 4) {
        return std::nullopt;
    }

    uint32_t size = stream.getBe32U();
    if (size != stream.leftBytes()) {
        return std::nullopt;
    }

    uint8_t uint8 = stream.peek8U();
    if ((uint8 >> 7) != 0) {
        return std::nullopt;
    }

    int oldSize = pendingData.size();
    if (oldSize != 0)
        oldSize -= 64;

    uint32_t nalHeader = 0x1000000;
    pendingData.resize(oldSize + 4 + size + 64);
    memcpy(pendingData.data() + oldSize, &nalHeader, 4);
    stream.read(pendingData.data() + oldSize + 4, size);

    memset(pendingData.data() + oldSize + 4 + size, 0, 64);

    if (((uint8 >> 1) & 0b111111) < 0x20) {
        std::pair<int64_t, int64_t> ptsDts;
        try {
            ptsDts = mmtStream->calcPtsDts();
        }
        catch (const std::out_of_range& e) {
            std::cerr << e.what() << std::endl;
            return std::nullopt;
        }

        mmtStream->incrementAuIndex();

        MpuData mpuData;
        mpuData.data = std::move(pendingData);
        mpuData.pts = ptsDts.first;
        mpuData.dts = ptsDts.second;
        mpuData.streamIndex = mmtStream->streamIndex;
        mpuData. flags = mmtStream->flags;

        mmtStream->flags = 0;
        return mpuData;
    }

	return std::nullopt;
}