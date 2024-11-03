#pragma once
#include "mmtDescriptorBase.h"
#include <list>

namespace MmtTlv {

class ContentCopyControlDescriptor
    : public MmtDescriptorTemplate<0x8038> {
public:
    bool unpack(Common::Stream& stream) override;
    
    class Component {
    public:
        bool unpack(Common::Stream& stream);

        uint16_t componentTag;
        uint8_t digitalRecordingControlData;
        uint8_t maximumBitrateFlag;
        uint8_t reservedFutureUse1;
        uint8_t reservedFutureUse2;
        uint8_t maximumBitrate;

    };

    uint8_t digitalRecordingControlData;
    bool maximumBitrateFlag;
    bool componentControlFlag;
    uint8_t reservedFutureUse1;
    uint8_t reservedFutureUse2;

    uint8_t maximumBitrate;

    uint8_t componentControlLength;

    std::list<Component> components;
};

}