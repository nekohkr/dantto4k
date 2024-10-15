#pragma once
#include "mmtDescriptor.h"

class VideoComponentDescriptor
    : public MmtDescriptor<0x8010> {
public:
	bool unpack(Stream& stream) override;

    uint8_t videoResolution;
    uint8_t videoAspectRatio;
    bool videoScanFlag;
    uint8_t videoFrameRate;
    uint16_t componentTag;
    uint8_t videoTransferCharacteristics;
    char language[4];
    std::string text;
};