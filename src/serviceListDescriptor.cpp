#include "serviceListDescriptor.h"

namespace MmtTlv {

bool ServiceListDescriptor::unpack(Common::ReadStream& stream)
{
    try {
        if (!TlvDescriptorTemplate::unpack(stream)) {
            return false;
        }

        Common::ReadStream nstream(stream, descriptorLength);
        while (!nstream.isEof()) {
            Entry item;
            item.unpack(nstream);
            services.push_back(item);
        }
        stream.skip(descriptorLength);
	}
	catch (const std::out_of_range&) {
		return false;
	}

    return true;
}

bool ServiceListDescriptor::Entry::unpack(Common::ReadStream& stream)
{
    serviceId = stream.getBe16U();
    serviceType = stream.get8U();
    return true;
}

}