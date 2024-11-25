#include "subtitleMfuDataProcessor.h"

namespace MmtTlv {

std::optional<MfuData> SubtitleMfuDataProcessor::process(const std::shared_ptr<MmtStream>& mmtStream, const std::vector<uint8_t>& data)
{
    Common::ReadStream stream(data);

    uint16_t subsampleNumber = stream.getBe16U();
    uint16_t lastSubsampleNumber = stream.getBe16U();

    uint8_t uint8 = stream.get8U();
    uint8_t dataType = uint8 >> 4;
    uint8_t lengthExtFlag = (uint8 >> 3) & 1;
    uint8_t subsampleInfoListFlag = (uint8 >> 2) & 1;

    if (dataType != 0) {
        return std::nullopt;
    }

    uint32_t dataSize;
    if (lengthExtFlag)
        dataSize = stream.getBe32U();
    else
        dataSize = stream.getBe16U();

    if (subsampleNumber == 0 && lastSubsampleNumber > 0 && subsampleInfoListFlag) {
        for (int i = 0; i < lastSubsampleNumber; ++i) {
            stream.skip(4 + 4); // subsample_i_data_type

            if (lengthExtFlag) {
                stream.skip(4); // subsample_i_data_size
            }
            else {
                stream.skip(2); // subsample_i_data_size
            }
        }
    }

    if (stream.leftBytes() < dataSize) {
        return std::nullopt;
    }

    MfuData mfuData;
    mfuData.data.resize(dataSize);
    stream.read(mfuData.data.data(), dataSize);

    mfuData.streamIndex = mmtStream->getStreamIndex();

    return mfuData;
}

}