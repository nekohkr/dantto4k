#include "mhAudioComponentDescriptor.h"

namespace MmtTlv {

bool MhAudioComponentDescriptor::unpack(Common::Stream& stream)
{
    try {
        if (!MmtDescriptorTemplate::unpack(stream)) {
            return false;
        }

        Common::Stream nstream(stream, descriptorLength);

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

        int textLength = nstream.leftBytes();
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

}