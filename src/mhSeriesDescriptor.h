#pragma once
#include "mmtDescriptorBase.h"

namespace MmtTlv {

class MhSeriesDescriptor
    : public MmtDescriptorTemplate<0x8016> {
public:
    bool unpack(Common::Stream& stream) override;

    uint16_t seriesId;

    uint8_t repeatLabel;
    uint8_t programPattern;
    bool expireDateValidFlag;

    uint16_t expireDate;

    uint16_t episodeNumber;
    uint16_t lastEpisodeNumber;

    std::string seriesNameChar;

};

}