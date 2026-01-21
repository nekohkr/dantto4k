#pragma once
#include "mmtTableBase.h"
#include <list>
#include <optional>

namespace MmtTlv {

// Data Directory Management Table
class Ddmt : public MmtTableBase {
public:
    bool unpack(Common::ReadStream& stream);

    class File {
    public:
        bool unpack(Common::ReadStream& stream);

        uint16_t nodeTag;
        std::string fileName;

    };

    class Node {
    public:
        bool unpack(Common::ReadStream& stream);

        uint16_t nodeTag;
        uint8_t directoryNodeVersion;
        std::string directoryNodePath;
        std::list <File> files;
    };

    bool sectionSyntaxIndicator;
    uint16_t sectionLength;
    uint8_t dataTransmissionSessionId;
    uint8_t versionNumber;
    bool currentNextIndicator;
    uint8_t sectionNumber;
    uint8_t lastSectionNumber;

    std::string baseDirectoryPath;
    std::list<Node> nodes;

    uint32_t crc32;

};

}