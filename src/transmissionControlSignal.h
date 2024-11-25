#pragma once
#include "stream.h"

namespace MmtTlv {

class TransmissionControlSignal
{
public:
	bool unpack(Common::ReadStream& stream);

public:
	uint8_t tableId;
	uint16_t sectionSyntaxIndicator;
	uint16_t sectionLength;
	uint16_t tableIdExtension;
	bool currentNextIndicator;
	uint8_t sectionNumber;
	uint8_t lastSectionNumber;
};
}