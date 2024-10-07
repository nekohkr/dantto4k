#pragma once
#include "mmtDescriptor.h"

class MhAudioComponentDescriptor : public MmtDescriptor {
public:
    bool unpack(Stream& stream);

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
