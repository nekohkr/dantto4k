#include "mpuApplicationProcessor.h"

namespace MmtTlv {

std::optional<MpuData> MpuApplicationProcessor::process(const std::shared_ptr<MmtStream>& mmtStream, const std::vector<uint8_t>& data) {
    Common::ReadStream stream(data);
    size_t size = stream.leftBytes();
    if (size == 0) {
        return std::nullopt;
    }

    MpuData mfuData;
    mfuData.data.resize(size);
    stream.read(mfuData.data.data(), size);

    mfuData.streamIndex = mmtStream->getStreamIndex();

    return mfuData;
}

}