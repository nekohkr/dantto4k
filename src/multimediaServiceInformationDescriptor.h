#pragma once
#include "mmtDescriptorBase.h"

namespace MmtTlv {

class MultimediaServiceInformationDescriptor
	: public MmtDescriptorTemplate<0x803F> {
public:
	bool unpack(Common::ReadStream& stream) override;

	uint16_t dataComponentId;

	uint16_t componentTag;
	char language[4];
	uint8_t textLength;
	std::string text;

	uint8_t associatedContentsFlag;
	uint8_t reserved;

	uint8_t selectorLength;
	std::vector<uint8_t> selectorByte;

};

}