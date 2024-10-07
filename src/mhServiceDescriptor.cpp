#include "mhServiceDescriptor.h"

bool MhServiceDescriptor::unpack(Stream& stream)
{
	if (stream.leftBytes() < 3) {
		return false;
	}

	if (!MmtDescriptor::unpack(stream)) {
		return false;
	}

	serviceType = stream.get8U();
	serviceProviderNameLength = stream.get8U();
	serviceProviderName.resize(serviceProviderNameLength + 1);
	stream.read(serviceProviderName.data(), serviceProviderNameLength);
	serviceProviderName.data()[serviceProviderNameLength] = 0;

	serviceNameLength = stream.get8U();
	serviceName.resize(serviceNameLength + 1);
	stream.read(serviceName.data(), serviceNameLength);
	serviceName.data()[serviceNameLength] = 0;

	return true;
}