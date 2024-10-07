#include "mhSdt.h"
#include "mhServiceDescriptor.h"

MhSdt::~MhSdt()
{
    for (auto service : services) {
        delete service;
    }
}

bool MhSdt::unpack(Stream& stream)
{
    if (!MmtTable::unpack(stream)) {
        return false;
    }

    if (stream.leftBytes() < 2 + 2 + 1 + 1 + 1 + 2 + 1 + 4) {
        return false;
    }

    uint16_t uint16 = stream.getBe16U();
    sectionSyntaxIndicator = (uint16 & 0b1000000000000000) >> 15;
    sectionLength = uint16 & 0b0000111111111111;

    tlvStreamId = stream.getBe16U();

    uint8_t uint8 = stream.get8U();
    currentNextIndicator = uint8 & 1;
    sectionNumber = stream.get8U();
    lastSectionNumber = stream.get8U();
    originalNetworkId = stream.getBe16U();
    stream.skip(1);
    while (stream.leftBytes() > 4) {
        MhSdtService* service = new MhSdtService();
        service->unpack(stream);
        services.push_back(service);

        int a = stream.leftBytes();
        a = 0;
    }

    crc32 = stream.getBe32U();

	return true;
}

MhSdtService::~MhSdtService()
{
    for (auto descriptor : descriptors) {
        delete descriptor;
    }
    descriptors.clear();
}

bool MhSdtService::unpack(Stream& stream)
{
    if (stream.leftBytes() < 2 + 1 + 2) {
        return false;
    }

    serviceId = stream.getBe16U();

    uint8_t uint8 = stream.get8U();
    eitUserDefinedFlags = (uint8 & 0b00011100) >> 2;
    eitScheduleFlag = (uint8 & 0b00000010) >> 1;
    eitPresentFollowingFlag = (uint8 & 0b00000001) >> 1;

    uint16_t uint16 = stream.getBe16U();
    runningStatus = (uint16 & 0b1110000000000000) >> 13;
    freeCaMode = (uint16 & 0b0001000000000000) >> 12;
    descriptorsLoopLength = uint16 & 0b0000111111111111;


    if (stream.leftBytes() < descriptorsLoopLength) {
        return false;
    }

    Stream nstream(stream, descriptorsLoopLength);
    while (!nstream.isEOF()) {
        uint16_t descriptorTag = nstream.peekBe16U();
        switch (descriptorTag) {
        case MH_SERVICE_DECRIPTOR:
        {
            MhServiceDescriptor* descriptor = new MhServiceDescriptor();
            if (!descriptor->unpack(nstream)) {
                return false;
            }
            descriptors.push_back(descriptor);
            break;
        }
        default:
        {
            MmtDescriptor descriptor;
            descriptor.unpack(nstream);
            nstream.skip(descriptor.descriptorLength);
            break;
        }
        }
    }

    stream.skip(descriptorsLoopLength);
    return true;
}