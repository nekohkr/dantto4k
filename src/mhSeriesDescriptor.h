#pragma once
#include "mmtDescriptor.h"

class MhSeriesDescriptor
    : public MmtDescriptor<0x8016> {
public:
    bool unpack(Stream& stream) override;

    uint16_t seriesId;

    uint8_t repeatLabel; //4 bits
    uint8_t programPattern; //3 bits
    bool expireDateValidFlag; //1 bits

    uint16_t expireDate;

    uint16_t episodeNumber; //12 bits
    uint16_t lastEpisodeNumber; //12 bits

    std::string seriesNameChar;

};
