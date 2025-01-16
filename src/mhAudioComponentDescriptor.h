#pragma once
#include "mmtDescriptorBase.h"

namespace MmtTlv {

class MhAudioComponentDescriptor
    : public MmtDescriptorTemplate<0x8014> {
public:
    bool unpack(Common::ReadStream& stream) override;

    uint32_t getConvertedSamplingRate() const;
    uint8_t getDialogControl() const { return componentType & 0b10000000 >> 7; }
    uint8_t getAudioForHandicapped() const { return componentType & 0b01100000 >> 5; }
    uint8_t getAudioMode() const { return componentType & 0b00011111; }

    bool Is22_2chAudio() const { return getAudioMode() == 0b10001; }

    uint8_t streamContent;
    uint8_t componentType;
    uint16_t componentTag;
    uint8_t streamType;
    uint8_t simulcastGroupTag;
    bool esMultiLingualFlag;
    bool mainComponentFlag;
    uint8_t qualityIndicator;
    uint8_t samplingRate;
    char language1[4];
    char language2[4];
    std::string text;
};

}