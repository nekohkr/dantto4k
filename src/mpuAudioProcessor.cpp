#include "mpuAudioProcessor.h"

namespace MmtTlv {

std::optional<MpuData> MpuAudioProcessor::process(const std::shared_ptr<MmtStream>& mmtStream, const std::vector<uint8_t>& data) {
    Common::ReadStream stream(data);
    size_t size = stream.leftBytes();

    std::pair<int64_t, int64_t> ptsDts;
    try {
        ptsDts = mmtStream->getNextPtsDts();
    }
    catch (const std::out_of_range&) {
        return std::nullopt;
    }

    MpuData mfuData;
    mfuData.data.resize(size + 3);
    mfuData.data[0] = 0x56;
    mfuData.data[1] = ((size >> 8) & 0x1F) | 0xE0;
    mfuData.data[2] = size & 0xFF;
    stream.read(mfuData.data.data() + 3, size);

    mfuData.pts = ptsDts.first;
    mfuData.dts = ptsDts.second;
    mfuData.streamIndex = mmtStream->getStreamIndex();

	return mfuData;
}

}