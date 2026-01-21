#pragma once
#include "mmtDescriptorBase.h"
#include <vector>

namespace MmtTlv {

class MhTransportProtocolDescriptor
	: public MmtDescriptorTemplate<0x802A> {
public:
	bool unpack(Common::ReadStream& stream) override;

	uint16_t protocolId;
	uint8_t transportProtocolLabel;
	std::vector<uint8_t> selector;

};

}