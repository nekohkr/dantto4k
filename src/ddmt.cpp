#include "ddmt.h"

namespace MmtTlv {

bool Ddmt::File::unpack(Common::ReadStream& stream) {
    try {
        nodeTag = stream.getBe16U();

        uint8_t fileNameLength = stream.get8U();
        fileName.resize(fileNameLength);
        stream.read(fileName.data(), fileNameLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

bool Ddmt::Node::unpack(Common::ReadStream& stream) {
    try {
        nodeTag = stream.getBe16U();
        directoryNodeVersion = stream.get8U();

        uint8_t directoryNodePathLength = stream.get8U();
        directoryNodePath.resize(directoryNodePathLength);
        stream.read(directoryNodePath.data(), directoryNodePathLength);

        uint16_t numOfFiles = stream.getBe16U();
        for (int i = 0; i < numOfFiles; i++) {
            File file;
            if (!file.unpack(stream)) {
                return false;
            }
            files.push_back(file);
        }
    }
    catch (const std::out_of_range&) {
        return false;
    }
    return true;
}

bool Ddmt::unpack(Common::ReadStream& stream) {
    try {
        if (!MmtTableBase::unpack(stream)) {
            return false;
        }

        uint16_t uint16 = stream.getBe16U();
        sectionSyntaxIndicator = (uint16 & 0b1000000000000000) >> 15;
        sectionLength = uint16 & 0b0000111111111111;
        dataTransmissionSessionId = stream.get8U();
        stream.skip(1); // reserved

        uint8_t uint8 = stream.get8U();
        versionNumber = (uint8 & 0b00111110) >> 1;
        currentNextIndicator = uint8 & 1;
        sectionNumber = stream.get8U();
        lastSectionNumber = stream.get8U();

        uint8_t baseDirectoryPathLength = stream.get8U();
        baseDirectoryPath.resize(baseDirectoryPathLength); 
        stream.read(baseDirectoryPath.data(), baseDirectoryPathLength);

        uint16_t numOfDirectoryNodes = stream.get8U();
        for (int i = 0; i < numOfDirectoryNodes; i++) {
            Node node;
            if (!node.unpack(stream)) {
                return false;
            }
            nodes.push_back(node);
        }
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

}