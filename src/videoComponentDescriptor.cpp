#include "videoComponentDescriptor.h"

bool VideoComponentDescriptor::unpack(Stream& stream)
{
    try {
        if (!MmtDescriptor::unpack(stream)) {
            return false;
        }

        uint8_t uint8 = stream.get8U();
        videoResolution = (uint8 & 0b11110000) >> 4;
        videoAspectRatio = uint8 & 0b0001111;

        uint8 = stream.get8U();
        videoScanFlag = (uint8 & 0b10000000) >> 7;
        videoFrameRate = uint8 & 0b00011111;
        componentTag = stream.getBe16U();

        uint8 = stream.get8U();
        videoTransferCharacteristics = (uint8 & 0b11110000) >> 4;
        stream.read((char*)language, 3);
        language[3] = '\0';

        int textLength = stream.leftBytes();
        text.resize(textLength);
        stream.read((char*)text.data(), textLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

	return true;
}
