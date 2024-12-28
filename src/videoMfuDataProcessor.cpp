#include "videoMfuDataProcessor.h"
#include "stream.h"

namespace MmtTlv {

std::optional<MfuData> VideoMfuDataProcessor::process(const std::shared_ptr<MmtStream>& mmtStream, const std::vector<uint8_t>& data)
{
    Common::ReadStream stream(data);
    if (stream.leftBytes() < 4) {
#ifndef _DANTTO4K_DLL
        std::cerr << "MFU data appears to be corrupted." << std::endl;
#endif
        return std::nullopt;
    }

    uint32_t size = stream.getBe32U();
    if (size != stream.leftBytes()) {
#ifndef _DANTTO4K_DLL
        std::cerr << "MFU data appears to be corrupted." << std::endl;
#endif
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
        if (sliceSegmentCount >= (mmtStream->Is8KVideo() ? 3 : 0)) {
            std::pair<int64_t, int64_t> ptsDts;
            try {
                ptsDts = mmtStream->getNextPtsDts();
            }
            catch (const std::out_of_range&) {
                return std::nullopt;
            }

            MfuData mfuData;
            mfuData.data = std::move(pendingData);
            mfuData.pts = ptsDts.first;
            mfuData.dts = ptsDts.second;
            mfuData.streamIndex = mmtStream->getStreamIndex();
            
            if (nalUnitType == 0x15 /* CRA_NUT */) {
                mfuData.keyframe = true;
            }

            return mfuData;
        }
        sliceSegmentCount++;
    }

    if (nalUnitType == 0x23 /* NAL_AUD */) {
        sliceSegmentCount = 0;
    }

	return std::nullopt;
}

void VideoMfuDataProcessor::appendPendingData(Common::ReadStream& stream, int size)
{
    uint32_t nalStartCode = 0x1000000;
    size_t oldSize = pendingData.size();
    pendingData.resize(oldSize + 4 + size);

    memcpy(pendingData.data() + oldSize, &nalStartCode, 4);
    stream.read(pendingData.data() + oldSize + 4, size);
}

}