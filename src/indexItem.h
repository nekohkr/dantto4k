#pragma once
#include <cstdint>
#include <string>
#include <list>
#include "stream.h"

namespace MmtTlv {

class IndexItem {
public:
    bool unpack(Common::ReadStream& stream);

    class Item {
    public:
        bool unpack(Common::ReadStream& stream);

        uint32_t itemId;
        uint32_t itemSize;
        uint8_t itemVersion;
        std::string fileName;
        bool checksumFlag;
        uint32_t itemChecksum;
        std::string itemType;
        uint8_t compressionType;
        uint32_t originalSize;
    };

    std::list<Item> items;
};

}