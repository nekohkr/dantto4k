#include "tlvNit.h"

bool TlvNit::unpack(Stream& stream)
{
    if (!TlvTable::unpack(stream)) {
        return false;
    }

    uint16_t uint16 = stream.getBe16U();
    networkDescriptorsLength = uint16 & 0b0000111111111111;
    

    Stream nstream(stream, networkDescriptorsLength);
    while (!nstream.isEOF()) {
        uint8_t descriptorTag = nstream.peek8U();
        switch (descriptorTag) {
        case 0x40://network name
        {
            NetworkNameDescriptor* descriptor = new NetworkNameDescriptor();
            descriptor->unpack(nstream);
            descriptors.push_back(descriptor);
            break;
        }
        case 0x41://service list
        {
            ServiceListDescriptor* descriptor = new ServiceListDescriptor();
            descriptor->unpack(nstream);
            descriptors.push_back(descriptor);
            break;
        }
        case 0xCD://remote control key
        {
            RemoteControlKeyDescriptor* descriptor = new RemoteControlKeyDescriptor();
            descriptor->unpack(nstream);
            descriptors.push_back(descriptor);
            break;
        }
        default:
        {
            TlvDescriptor descriptor;
            descriptor.unpack(nstream);
            nstream.skip(descriptor.descriptorLength);
        }
        }
    }
    stream.skip(networkDescriptorsLength);

    uint16 = stream.getBe16U();
    tlvStreamLoopLength = uint16 & 0b0000111111111111;

    Stream tlvStream(stream, tlvStreamLoopLength);
    while (!tlvStream.isEOF()) {
        TlvNitItem item;
        item.unpack(tlvStream);
        items.push_back(item);
    }
    stream.skip(tlvStreamLoopLength);
	return true;
}

bool TlvDescriptor::unpack(Stream& stream)
{
    descriptorTag = stream.get8U();
    descriptorLength = stream.get8U();
    return true;
}

bool ServiceListDescriptor::unpack(Stream& stream)
{
    if (!TlvDescriptor::unpack(stream)) {
        return false;
    }

    Stream nstream(stream, descriptorLength);
    while (!nstream.isEOF()) {
        ServiceListItem item;
        item.unpack(nstream);
        services.push_back(item);
    }
    stream.skip(descriptorLength);
    return true;
}

bool ServiceListItem::unpack(Stream& stream)
{
    serviceId = stream.getBe16U();
    serviceType = stream.get8U();
    return true;
}

bool NetworkNameDescriptor::unpack(Stream& stream)
{
    if (!TlvDescriptor::unpack(stream)) {
        return false;
    }

    int size = stream.leftBytes();
    networkName.resize(size);
    stream.read(networkName.data(), size);
    return true;
}

bool RemoteControlKeyItem::unpack(Stream& stream)
{
    remoteControlKeyId = stream.get8U();
    serviceId = stream.getBe16U();
    return true;
}

bool RemoteControlKeyDescriptor::unpack(Stream& stream)
{
    if (!TlvDescriptor::unpack(stream)) {
        return false;
    }

    numOfRemoteControlKeyId = stream.get8U();
    for (int i = 0; i < numOfRemoteControlKeyId; i++) {
        RemoteControlKeyItem item;
        item.unpack(stream);
        items.push_back(item);
    }

    return true;
}

bool TlvNitItem::unpack(Stream& stream)
{
    tlvStreamId = stream.getBe16U();
    originalNetworkId = stream.getBe16U();

    uint16_t uint16 = stream.getBe16U();
    tlvStreamDescriptorsLength = uint16 & 0b0000111111111111;

    Stream nstream(stream, tlvStreamDescriptorsLength);
    while (!nstream.isEOF()) {
        uint8_t descriptorTag = nstream.peek8U();
        switch (descriptorTag) {
        case 0x40://network name
        {
            NetworkNameDescriptor* descriptor = new NetworkNameDescriptor();
            descriptor->unpack(nstream);
            descriptors.push_back(descriptor);
            break;
        }
        case 0x41://service list
        {
            ServiceListDescriptor* descriptor = new ServiceListDescriptor();
            descriptor->unpack(nstream);
            descriptors.push_back(descriptor);
            break;
        }
        case 0xCD://remote control key
        {
            RemoteControlKeyDescriptor* descriptor = new RemoteControlKeyDescriptor();
            descriptor->unpack(nstream);
            descriptors.push_back(descriptor);
            break;
        }
        default:
        {
            TlvDescriptor descriptor;
            descriptor.unpack(nstream);
            nstream.skip(descriptor.descriptorLength);
        }
        }
    }
    stream.skip(tlvStreamDescriptorsLength);
    return true;
}

bool TlvTable::unpack(Stream& stream)
{
    tableId = stream.get8U();

    uint16_t uint16 = stream.getBe16U();
    sectionSyntaxIndicator = (uint16 & 0b1000000000000000) >> 15;
    sectionLength = uint16 & 0b0000111111111111;

    networkId = stream.getBe16U();

    uint8_t uint8 = stream.get8U();
    currentNextIndicator = uint8 & 1;
    sectionNumber = stream.get8U();
    lastSectionNumber = stream.get8U();
    return true;
}
