#pragma once
#include <vector>
#include "mmtTableBase.h"
#include "mmtDescriptors.h"

namespace MmtTlv {

// MH-Broadcaster_Information_Table
class MhBit : public MmtTableBase {
public:
    bool unpack(Common::ReadStream& stream);

    class Broadcaster {
    public:
        bool unpack(Common::ReadStream& stream);

        uint8_t broadcasterId;
        uint16_t broadcasterDescriptorsLength;
        MmtDescriptors descriptors;
    };

    uint16_t sectionSyntaxIndicator;
    uint16_t sectionLength;

    uint16_t originalNetworkId;
    uint8_t versionNumber;
    bool currentNextIndicator;
    uint8_t sectionNumber;
    uint8_t lastSectionNumber;

    bool broadcastViewPropriety;
    uint16_t firstDescriptorsLength;

    MmtDescriptors descriptors;


    std::vector<Broadcaster> broadcasters;
    uint32_t crc32;
};

}