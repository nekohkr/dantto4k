#include "mhServiceDescriptor.h"

namespace MmtTlv {

bool MhServiceDescriptor::unpack(Common::ReadStream& stream)
{
	try {
		if (!MmtDescriptorTemplate::unpack(stream)) {
			return false;
		}
		
        Common::ReadStream nstream(stream, descriptorLength);

		serviceType = nstream.get8U();
		serviceProviderNameLength = nstream.get8U();
		if (serviceProviderNameLength) {
			serviceProviderName.resize(serviceProviderNameLength);
			nstream.read(serviceProviderName.data(), serviceProviderNameLength);
		}

		serviceNameLength = nstream.get8U();
		if (serviceNameLength) {
			serviceName.resize(serviceNameLength);
			nstream.read(serviceName.data(), serviceNameLength);
		}

		stream.skip(descriptorLength);
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

}