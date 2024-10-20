#include "videoComponentDescriptor.h"

bool VideoComponentDescriptor::unpack(Stream& stream)
{
    try {
        if (!MmtDescriptor::unpack(stream)) {
            return false;
        }

        Stream nstream(stream, descriptorLength);

        uint8_t uint8 = nstream.get8U();
        videoResolution = (uint8 & 0b11110000) >> 4;
        videoAspectRatio = uint8 & 0b0001111;

        uint8 = nstream.get8U();
        videoScanFlag = (uint8 & 0b10000000) >> 7;
        videoFrameRate = uint8 & 0b00011111;
        componentTag = nstream.getBe16U();

        uint8 = nstream.get8U();
        videoTransferCharacteristics = (uint8 & 0b11110000) >> 4;
        nstream.read(language, 3);
        language[3] = '\0';

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
