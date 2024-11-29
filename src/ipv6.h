#pragma once
#include "stream.h"
#include "ip.h"

namespace MmtTlv {

namespace IPv6 {
	constexpr uint8_t PROTOCOL_UDP = 17;
	constexpr uint16_t PORT_NTP = 123;
}

class IPv6Header {
public:
	IPv6Header(bool isCompressed = false) :
		isCompressed(isCompressed) {}

	bool unpack(Common::ReadStream& stream);

public:
	bool isCompressed{};
	uint8_t version{};
	uint8_t priority{};
	uint32_t flow_lbl{};
	uint8_t nexthdr{};
	uint8_t hop_limit{};


	uint16_t payloadLength{};
	uint8_t payloadType{};

	struct Common::in6_addr saddr;
	struct Common::in6_addr daddr;
};

class IPv6ExtensionHeader {
public:
	bool unpack(Common::ReadStream& stream, bool headerLengthOnly=false);

public:
	uint8_t next_header;
	uint8_t header_length;

};

class UDPHeader {
public:
	bool unpack(Common::ReadStream& stream, bool headerLengthOnly = false);

public:
	uint16_t source_port;
	uint16_t destination_port;
	uint16_t length;
	uint16_t checksum;
};
}