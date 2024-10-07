#pragma once
#include "mmtDescriptor.h"

class MhServiceDescriptor : public MmtDescriptor {
public:
	virtual ~MhServiceDescriptor() {}
	bool unpack(Stream& stream);

	uint8_t serviceType;
	uint8_t serviceProviderNameLength;
	std::string serviceProviderName;
	uint8_t serviceNameLength;
	std::string serviceName;
};