#include "applicationMfuDataProcessor.h"

namespace MmtTlv {

std::optional<MfuData> ApplicationMfuDataProcessor::process(const std::shared_ptr<MmtStream>& mmtStream, const std::vector<uint8_t>& data)
{
    Common::Stream stream(data);
    uint32_t size = stream.leftBytes();

    MfuData mfuData;
    mfuData.data.resize(size);
    stream.read(mfuData.data.data(), size);

    mfuData.streamIndex = mmtStream->getStreamIndex();

    return mfuData;
}

}