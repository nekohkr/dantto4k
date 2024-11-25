#include "mhServiceDescriptor.h"

namespace MmtTlv {

bool MhServiceDescriptor::unpack(Common::ReadStream& stream)
{
	try {
		if (!MmtDescriptorTemplate::unpack(stream)) {
			return false;
		}

		serviceType = stream.get8U();
		serviceProviderNameLength = stream.get8U();
		if (serviceProviderNameLength) {
			serviceProviderName.resize(serviceProviderNameLength);
			stream.read(serviceProviderName.data(), serviceProviderNameLength);
		}

		serviceNameLength = stream.get8U();
		if (serviceNameLength) {
			serviceName.resize(serviceNameLength);
			stream.read(serviceName.data(), serviceNameLength);
		}
	}
	catch (const std::out_of_range&) {
		return false;
	}
	return true;
}

}