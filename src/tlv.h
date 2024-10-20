#pragma once
#include <vector>
#include "ip.h"

class StreamBase;
class Stream;
class MmtTlvDemuxer;

enum TLV_PACKET_TYPE
{
	TLV_UNDEFINED = 0x0,
	TLV_IPv4_PACKET = 0x01,
	TLV_IPv6_PACKET = 0x02,
	TLV_HEADER_COMPRESSED_IP_PACKET = 0x03,
	TLV_TRANSMISSION_CONTROL_SIGNAL_PACKET = 0xFE,
	TLV_NULL_PACKET = 0xFF
};

enum CONTEXT_HEADER_TYPE
{
	CONTEXT_ID_PARTIAL_IPV4_HEADER_AND_PARTIAL_UDP_HEADER = 0x20,
	CONTEXT_ID_IPV4_HEADER_IDENTIFIER = 0x21,
	CONTEXT_ID_PARTIAL_IPV6_HEADER_AND_PARTIAL_UDP_HEADER = 0x60,
	CONTEXT_ID_NO_COMPRESSED_HEADER = 0x61,
};

class TransmissionControlSignal
{
public:
	bool unpack(Stream& stream);

public:
	uint8_t tableId;
	uint16_t sectionSyntaxIndicator;
	uint16_t sectionLength;
	uint16_t tableIdExtension;
	bool currentNextIndicator;
	uint8_t sectionNumber;
	uint8_t lastSectionNumber;
};


class IPv6Header {
public:
	bool unpack(Stream& stream);

protected:
	uint8_t version;
	uint8_t priority;
	uint32_t flow_lbl;
	uint8_t nexthdr;
	uint8_t hop_limit;

	struct nn_in6_addr saddr;
	struct nn_in6_addr daddr;

};

class CompressedIPPacket {
public:
	bool unpack(Stream& stream);

	std::vector<uint8_t> getCompressedHeader() const {
		return compressedHeader;
	}

public:
	std::vector<uint8_t> compressedHeader;
	uint16_t contextId;
	uint8_t sequenceNumber;
	uint8_t headerType;
	std::vector<uint8_t> ipv6;
	std::vector<uint8_t> udp;
	std::vector<uint8_t> ipv4;
};

class TLVPacket {
public:
	bool unpack(StreamBase& stream);

	const std::vector<uint8_t>& getData() const {
		return data;
	}

	MmtTlvDemuxer* demux;

	std::vector<uint8_t> data;
	uint8_t packetType;
	uint16_t dataLength;

protected:
	CompressedIPPacket compressedIPPacket;
};