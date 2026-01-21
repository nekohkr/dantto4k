#pragma once
#include "mmtTableBase.h"
#include <list>
#include <optional>

namespace MmtTlv {

// Data Asset Management Table
class Damt : public MmtTableBase {
public:
    bool unpack(Common::ReadStream& stream);

    class Item {
    public:
        bool unpack(Common::ReadStream& stream, bool indexItemFlag);

        uint16_t nodeTag;
        uint32_t itemId;
        uint32_t itemSize;
        uint8_t itemVersion;
        bool checksumFlag;
        uint32_t itemChecksum;
        std::vector<uint8_t> itemInfo;

    };

    class Mpu {
    public:
        bool unpack(Common::ReadStream& stream);

        uint32_t mpuSequenceNumber;
        uint32_t mpuSize;
        bool indexItemFlag;
        bool indexItemIdFlag;
        uint8_t indexItemCompressionFlag;
        uint32_t indexItemId{0};
        uint16_t numOfItems;
        std::list<Item> items;
        std::optional<uint16_t> nodeTag;

    };

    bool sectionSyntaxIndicator;
    uint16_t sectionLength;
    uint8_t dataTransmissionSessionId;
    uint8_t versionNumber;
    bool currentNextIndicator;
    uint8_t sectionNumber;
    uint8_t lastSectionNumber;
    uint32_t transactionId;
    uint16_t componentTag;
    uint32_t downloadId;
    std::list<Mpu> mpus;
    std::vector<uint8_t> componentInfo;
    uint32_t crc32;

};

}