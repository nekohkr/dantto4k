#include "ttmlMpuDataProcessor.h"

std::optional<MpuData> TtmlMpuDataProcessor::process(const std::shared_ptr<MmtStream>& mmtStream, const std::vector<uint8_t>& data)
{
    Stream stream(data);
    uint32_t size = stream.leftBytes();

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
            // skip: subsample_i_data_type
            stream.skip(4 + 4);

            // skip: subsample_i_data_size
            if (lengthExtFlag) {
                stream.skip(4);
            }
            else {
                stream.skip(2);
            }
        }
    }

    if (stream.leftBytes() < dataSize) {
        return std::nullopt;
    }

    MpuData mpuData;
    mpuData.data.resize(dataSize);
    stream.read(mpuData.data.data(), dataSize);

    mpuData.streamIndex = mmtStream->streamIndex;
    mpuData.flags = mmtStream->flags;

    mmtStream->flags = 0;

    return mpuData;
}