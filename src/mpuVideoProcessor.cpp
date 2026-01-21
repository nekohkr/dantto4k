#include "mpuVideoProcessor.h"
#include "stream.h"
#include <iostream>

namespace MmtTlv {

constexpr uint8_t CRA_NUT = 0x15;
constexpr uint8_t NAL_AUD = 0x23;

std::optional<MfuData> MpuVideoProcessor::process(MmtStream& mmtStream, const std::vector<uint8_t>& data) {
    Common::ReadStream stream(data);
    MfuData mfuData;

    if (nalUnitSize == 0) {
        if (stream.leftBytes() < 4) {
            return std::nullopt;
        }

        nalUnitSize = stream.getBe32U();
        uint8_t uint8 = stream.peek8U();
        nalUnitType = ((uint8 >> 1) & 0b111111);

        if (nalUnitType == NAL_AUD) {
            std::pair<int64_t, int64_t> ptsDts;
            try {
                ptsDts = mmtStream.getNextPtsDts();
            }
            catch (const std::out_of_range&) {
                clear();
                return std::nullopt;
            }

            pts = ptsDts.first;
            dts = ptsDts.second;
            streamIndex = mmtStream.getStreamIndex();
            sliceSegmentCount = 0;
            mfuData.isFirstFragment = true;
        }

        static const uint8_t startCode[4] = { 0x00, 0x00, 0x00, 0x01 };
        buffer.insert(buffer.end(), startCode, startCode + 4);

        if (nalUnitType < 0x20) {
            sliceSegmentCount++;
        }
        if (nalUnitType == CRA_NUT) {
            mfuData.keyframe = true;
        }
    }

    size_t oldSize = buffer.size();
    size_t remain = stream.leftBytes();
    if (nalUnitSize < remain) {
        clear();
        return std::nullopt;
    }

    nalUnitSize -= remain;
    buffer.resize(oldSize + remain);
    stream.read(buffer.data() + oldSize, remain);

    mfuData.pts = pts;
    mfuData.dts = dts;
    mfuData.streamIndex = streamIndex;

    if (sliceSegmentCount >= (mmtStream.is8KVideo() ? 4 : 1)) {
        if (nalUnitSize == 0) {
            mfuData.isLastFragment = true;
        }
    }

    mfuData.data = std::move(buffer);
    return mfuData;
}

void MpuVideoProcessor::clear() {
    sliceSegmentCount = 0;
    nalUnitSize = 0;
    nalUnitType = 0;
    pts = 0;
    dts = 0;
    streamIndex = 0;
    buffer.clear();

}

}