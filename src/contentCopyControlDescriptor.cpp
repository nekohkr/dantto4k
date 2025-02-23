#include "contentCopyControlDescriptor.h"

namespace MmtTlv {

bool ContentCopyControlDescriptor::unpack(Common::ReadStream& stream)
{
    try {
        if (!MmtDescriptorTemplate::unpack(stream)) {
            return false;
        }

        Common::ReadStream nstream(stream, descriptorLength);

        uint8_t uint8 = nstream.get8U();
        digitalRecordingControlData = (uint8 & 0b11000000) >> 6;
        maximumBitrateFlag = (uint8 & 0b00100000) >> 5;
        componentControlFlag = (uint8 & 0b00010000) >> 4;
        reservedFutureUse1 = uint8 & 0b00001111;

        if (maximumBitrateFlag) {
            maximumBitrate = nstream.get8U();
        }

        if (componentControlFlag) {
            componentControlLength = nstream.get8U();
            
            Common::ReadStream componentStream(nstream, componentControlLength);
            while (componentStream.isEof()) {
                Component component;
                if (!component.unpack(componentStream)) {
                    return false;
                }
                components.push_back(component);
            }
        }

        stream.skip(descriptorLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

bool ContentCopyControlDescriptor::Component::unpack(Common::ReadStream& stream)
{
    try {
        componentTag = stream.getBe16U();
        uint8_t uint8 = stream.get8U();
        digitalRecordingControlData = (uint8 & 0b11000000) >> 6;
        maximumBitrateFlag = (uint8 & 0b00100000) >> 5;
        reservedFutureUse1 = uint8 & 0b00011111;
        reservedFutureUse2 = stream.get8U();

        if (maximumBitrateFlag) {
            maximumBitrate = stream.get8U();
        }
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

}