#include "videoMfuDataProcessor.h"
#include "stream.h"
#include "mmttlvdemuxer.h"

namespace MmtTlv {

std::optional<MfuData> VideoMfuDataProcessor::process(const std::shared_ptr<MmtStream>& mmtStream, const std::vector<uint8_t>& data)
{
    Common::Stream stream(data);
    if (stream.leftBytes() < 4) {
        std::cerr << "MFU data appears to be corrupted." << std::endl;
        return std::nullopt;
    }

    uint32_t size = stream.getBe32U();
    if (size != stream.leftBytes()) {
        std::cerr << "MFU data appears to be corrupted." << std::endl;
        return std::nullopt;
    }

    uint8_t uint8 = stream.peek8U();
    
    int forbiddenZeroBit = uint8 >> 7;

    if (forbiddenZeroBit != 0) {
        return std::nullopt;
    }
    
    int nalUnitType = ((uint8 >> 1) & 0b111111);
    
    appendPendingData(stream, size);

    if (nalUnitType < 0x20) {
        if (sliceSegmentCount >= (mmtStream->is8KVideo ? 3 : 0)) {
            std::pair<int64_t, int64_t> ptsDts;
            try {
                ptsDts = mmtStream->calcPtsDts();
            }
            catch (const std::out_of_range& e) {
                std::cerr << e.what() << std::endl;
                return std::nullopt;
            }

            mmtStream->incrementAuIndex();

            MfuData mfuData;
            mfuData.data = std::move(pendingData);
            mfuData.pts = ptsDts.first;
            mfuData.dts = ptsDts.second;
            mfuData.streamIndex = mmtStream->streamIndex;
            mfuData.flags = mmtStream->flags;

            mmtStream->flags = 0;

            return mfuData;
        }
        sliceSegmentCount++;
    }

    if (nalUnitType == 0x23 /* NAL_AUD */) {
        sliceSegmentCount = 0;
    }

	return std::nullopt;
}

void VideoMfuDataProcessor::appendPendingData(Common::Stream& stream, int size)
{
    uint32_t nalHeader = 0x1000000;
    int oldSize = pendingData.size();
    pendingData.resize(oldSize + 4 + size);

    memcpy(pendingData.data() + oldSize, &nalHeader, 4);
    stream.read(pendingData.data() + oldSize + 4, size);
}

}