#include "mhAudioComponentDescriptor.h"

namespace MmtTlv {

bool MhAudioComponentDescriptor::unpack(Common::ReadStream& stream)
{
    try {
        if (!MmtDescriptorTemplate::unpack(stream)) {
            return false;
        }

        Common::ReadStream nstream(stream, descriptorLength);

        uint8_t uint8 = nstream.get8U();
        streamContent = uint8 & 0b00001111;
        componentType = nstream.get8U();
        componentTag = nstream.getBe16U();
        streamType = nstream.get8U();
        simulcastGroupTag = nstream.get8U();

        uint8 = nstream.get8U();
        esMultiLingualFlag = (uint8 & 0b10000000) >> 7;
        mainComponentFlag = (uint8 & 0b01000000) >> 6;
        qualityIndicator = (uint8 & 0b00110000) >> 4;
        samplingRate = (uint8 & 0b00001110) >> 1;

        nstream.read(language1, 3);
        language1[3] = '\0';

        if (esMultiLingualFlag) {
            nstream.read(language2, 3);
            language2[3] = '\0';
        }

        size_t textLength = nstream.leftBytes();
        if (textLength) {
            text.resize(textLength);
            nstream.read(text.data(), textLength);
        }

        stream.skip(descriptorLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

uint32_t MhAudioComponentDescriptor::getConvertedSamplingRate() const
{
    switch (samplingRate) {
    case 0b001:
        return 16000;
    case 0b010:
        return 22050;
    case 0b011:
        return 24000;
    case 0b101:
        return 32000;
    case 0b110:
        return 44100;
    case 0b111:
        return 48000;
    }

    return 0;
}

}