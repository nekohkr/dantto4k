#include "mhServiceListDescriptor.h"

namespace MmtTlv {

bool MhServiceListDescriptor::unpack(Common::ReadStream& stream)
{
	try {
		if (!MmtDescriptorTemplate::unpack(stream)) {
			return false;
		}
		
        Common::ReadStream nstream(stream, descriptorLength);

		while (!nstream.isEof()) {
			Entry entry;
			if (!entry.unpack(nstream)) {
				return false;
			}

			entries.push_back(entry);
		}

		stream.skip(descriptorLength);
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

bool MhServiceListDescriptor::Entry::unpack(Common::ReadStream& stream)
{
	try {
		serviceId = stream.getBe16U();
		serviceType = stream.get8U();
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}
}