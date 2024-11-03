#pragma once
#include "mmtDescriptorBase.h"

namespace MmtTlv {

class MhServiceDescriptor
	: public MmtDescriptorTemplate<0x8019> {
public:
	virtual ~MhServiceDescriptor() {}
	bool unpack(Common::Stream& stream) override;

	uint8_t serviceType;
	uint8_t serviceProviderNameLength;
	std::string serviceProviderName;
	uint8_t serviceNameLength;
	std::string serviceName;
};

}