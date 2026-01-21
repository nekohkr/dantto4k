#pragma once
#include "mmtDescriptorBase.h"
#include <list>

namespace MmtTlv {

class MhApplicationDescriptor
    : public MmtDescriptorTemplate<0x8029> {
public:
    bool unpack(Common::ReadStream& stream) override;

    class ApplicationProfile {
    public:
        bool unpack(Common::ReadStream& stream);

        uint16_t applicationProfile;
        uint8_t versionMajor;
        uint8_t versionMinor;
        uint8_t versionMicro;
    };

    uint8_t applicationProfilesLength;
    std::list<ApplicationProfile> applicationProfiles;

    bool serviceBoundFlag;
    uint8_t visibility;
    bool presentApplicationPriority;
    uint8_t applicationPriority;
    std::vector<uint8_t> transportProtocolLabel;

};

}