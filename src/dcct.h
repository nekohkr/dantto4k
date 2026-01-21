#pragma once
#include "mmtTableBase.h"
#include <list>
#include <vector>

namespace MmtTlv {

// Data Content Configuration Table
class Dcct : public MmtTableBase {
public:
    bool unpack(Common::ReadStream& stream);

    class PU {
    public:
        bool unpack(Common::ReadStream& stream);

        uint16_t puTag;
        uint8_t puSize;
        std::list<uint16_t> nodeTags;
        std::vector<uint8_t> descriptor;
    };

    bool sectionSyntaxIndicator;
    uint16_t sectionLength;
    uint8_t dataTransmissionSessionId;
    uint8_t versionNumber;
    bool currentNextIndicator;
    uint8_t sectionNumber;
    uint8_t lastSectionNumber;

    uint16_t contentId;
    uint8_t contentVersion;
    uint32_t contentSize;
    bool puInfoFlag;
    bool contentInfoFlag;

    std::list<PU> pus;
    std::list<uint16_t> nodeTags;
    std::vector<uint8_t> descriptor;

    uint32_t crc32;

};

}