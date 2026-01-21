#include "damt.h"

namespace MmtTlv {

bool Damt::Item::unpack(Common::ReadStream& stream, bool indexItemFlag) {
    try {
        nodeTag = stream.getBe16U();
        if (!indexItemFlag) {
            itemId = stream.getBe32U();
            itemSize = stream.getBe32U();
            itemVersion = stream.get8U();
            
            uint8_t uint8 = stream.get8U();
            checksumFlag = (uint8 & 0b10000000) >> 7;
            if (checksumFlag) {
                itemChecksum = stream.getBe32U();
            }

            uint8_t itemInfoLength = stream.get8U();
            itemInfo.resize(itemInfoLength);
            stream.read(itemInfo.data(), itemInfoLength);
        }
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

bool Damt::Mpu::unpack(Common::ReadStream& stream) {
    try {
        mpuSequenceNumber = stream.getBe32U();
        mpuSize = stream.getBe32U();

        uint8_t uint8 = stream.get8U();
        indexItemFlag = (uint8 & 0b10000000) >> 7;
        indexItemIdFlag = (uint8 & 0b01000000) >> 6;
        indexItemCompressionFlag = (uint8 & 0b00110000) >> 4;

        if (indexItemFlag && indexItemIdFlag) {
            indexItemId = stream.getBe32U();
        }

        uint16_t numOfItems = stream.getBe16U();
        for (int i = 0; i < numOfItems; i++) {
            Item item;
            if (!item.unpack(stream, indexItemFlag)) {
                return false;
            }
            items.push_back(item);
        }

        uint8_t mpuInfoLength = stream.get8U();
        if (mpuInfoLength == 0x5) {
            stream.skip(3); // skip descriptor_tag and descriptor_length
            nodeTag = stream.getBe16U();
        }
        else {
            stream.skip(mpuInfoLength);
        }

    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

bool Damt::unpack(Common::ReadStream& stream) {
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
        transactionId = stream.getBe32U();
        componentTag = stream.getBe16U();
        downloadId = stream.getBe32U();

        uint16_t numOfMpus = stream.get8U();
        for (int i = 0; i < numOfMpus; i++) {
            Mpu mpu;
            if (!mpu.unpack(stream)) {
                return false;
            }
            mpus.push_back(mpu);
        }

        uint8_t componentInfoLength = stream.get8U();
        componentInfo.resize(componentInfoLength);
        stream.read(componentInfo.data(), componentInfoLength);

        crc32 = stream.getBe32U();
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

}

