#pragma once
#include <list>
#include "mmtTableBase.h"
#include "mmtDescriptors.h"

namespace MmtTlv {

// Mh-Service Description Table 
class MhSdt : public MmtTableBase {
public:
    bool unpack(Common::ReadStream& stream);

    class Service {
    public:
        bool unpack(Common::ReadStream& stream);

        uint16_t serviceId;
        int8_t eitUserDefinedFlags;
        bool eitScheduleFlag;
        bool eitPresentFollowingFlag;
        uint8_t runningStatus;
        bool freeCaMode;
        uint16_t descriptorsLoopLength;

        MmtDescriptors descriptors;
    };

    uint16_t sectionSyntaxIndicator;
    uint16_t sectionLength;

    uint16_t tlvStreamId;

    uint8_t versionNumber;
    bool currentNextIndicator;
    uint8_t sectionNumber;
    uint8_t lastSectionNumber;

    uint16_t originalNetworkId;

    std::list<std::shared_ptr<Service>> services;
    uint32_t crc32;
};

}