#pragma once
#include "mmtDescriptorBase.h"

namespace MmtTlv {

class VideoComponentDescriptor
    : public MmtDescriptorTemplate<0x8010> {
public:
	bool unpack(Common::Stream& stream) override;

    bool Is8KVideo() const { return videoResolution == 7; }

    uint8_t videoResolution;
    uint8_t videoAspectRatio;
    bool videoScanFlag;
    uint8_t videoFrameRate;
    uint16_t componentTag;
    uint8_t videoTransferCharacteristics;
    char language[4];
    std::string text;


};

}