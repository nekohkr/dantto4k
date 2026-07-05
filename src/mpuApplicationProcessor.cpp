#include "mpuApplicationProcessor.h"
#include "mmtStream.h"

namespace MmtTlv {

std::optional<MfuData> MpuApplicationProcessor::process(MmtStream& mmtStream, std::span<const uint8_t> data, FragmentationIndicator fragmentationIndicator) {
    Common::ReadStream stream(data);
    size_t size = stream.leftBytes();
    if (size == 0) {
        return std::nullopt;
    }

    MfuData mfuData;
    mfuData.data.resize(size);
    stream.read(mfuData.data.data(), size);

    mfuData.streamIndex = mmtStream.getStreamIndex();

    return mfuData;
}

}
