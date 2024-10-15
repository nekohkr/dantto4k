#pragma once
#include "mmtDescriptor.h"

class MhServiceDescriptor
	: public MmtDescriptor<0x8019> {
public:
	virtual ~MhServiceDescriptor() {}
	bool unpack(Stream& stream) override;

	uint8_t serviceType;
	uint8_t serviceProviderNameLength;
	std::string serviceProviderName;
	uint8_t serviceNameLength;
	std::string serviceName;
};