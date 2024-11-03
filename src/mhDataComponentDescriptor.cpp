#include "mhDataComponentDescriptor.h"

namespace MmtTlv {

bool MhDataComponentDescriptor::unpack(Common::Stream& stream)
{
	try {
		if (!MmtDescriptorTemplate::unpack(stream)) {
			return false;
		}

		Common::Stream nstream(stream, descriptorLength);

		dataComponentId = nstream.getBe16U();
		additionalDataComponentInfo.resize(nstream.leftBytes());
		nstream.read(additionalDataComponentInfo.data(), nstream.leftBytes());

		stream.skip(descriptorLength);
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

}