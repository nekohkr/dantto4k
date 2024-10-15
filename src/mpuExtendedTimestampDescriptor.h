#pragma once
#include "mmtDescriptor.h"


class MpuExtendedTimestampDescriptor
	: public MmtDescriptor<0x8026> {
public:
	bool unpack(Stream& stream) override;

	class Entry {
	public:
		bool unpack(Stream& stream, uint8_t ptsOffsetType, uint16_t defaultPtsOffset);

		uint32_t mpuSequenceNumber;
		uint8_t mpuPresentationTimeLeapIndicator;
		uint8_t reserved;
		uint16_t mpuDecodingTimeOffset;
		uint8_t numOfAu;
		std::vector<uint16_t> dtsPtsOffsets;
		std::vector<uint16_t> ptsOffsets;
	};

	uint8_t reserved;
	uint8_t ptsOffsetType;
	bool timescaleFlag;

	uint32_t timescale;
	uint16_t defaultPtsOffset;

	std::vector<Entry> entries;
};