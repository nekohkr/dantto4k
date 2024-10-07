#pragma once
#include "stream.h"
#include <list>

class TlvDescriptor {
public:
    bool unpack(Stream& stream);

    uint8_t descriptorTag;
    uint8_t descriptorLength;
};

class ServiceListItem {
public:
    bool unpack(Stream& stream);
    uint16_t serviceId;
    uint8_t serviceType;
};

class ServiceListDescriptor : public TlvDescriptor {
public:
    bool unpack(Stream& stream);
    std::list<ServiceListItem> services;
};

class NetworkNameDescriptor : public TlvDescriptor {
public:
    bool unpack(Stream& stream);
    std::string networkName;
};

class RemoteControlKeyItem {
public:
    bool unpack(Stream& stream);
    uint8_t remoteControlKeyId;
    uint16_t serviceId;

};

class RemoteControlKeyDescriptor : public TlvDescriptor {
public:
    bool unpack(Stream& stream);
    uint8_t numOfRemoteControlKeyId;
    std::list<RemoteControlKeyItem> items;
};

class TlvNitItem {
public:
    bool unpack(Stream& stream);

    uint16_t tlvStreamId;
    uint16_t originalNetworkId;
    uint16_t tlvStreamDescriptorsLength;
    std::list<TlvDescriptor*> descriptors;
};

class TlvTable {
public:
    bool unpack(Stream& stream);

    uint8_t tableId;
    uint16_t sectionSyntaxIndicator;
    uint16_t sectionLength;
    uint16_t networkId;
    bool currentNextIndicator;
    uint8_t sectionNumber;
    uint8_t lastSectionNumber;
};

class TlvNit : public TlvTable {
public:
    bool unpack(Stream& stream);


    uint16_t networkDescriptorsLength;
    std::list<TlvDescriptor*> descriptors;

    uint16_t tlvStreamLoopLength;
    std::list<TlvNitItem> items;

};