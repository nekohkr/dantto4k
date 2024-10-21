#include "mhDataComponentDescriptor.h"

bool MhDataComponentDescriptor::unpack(Stream& stream)
{
	try {
		if (!MmtDescriptor::unpack(stream)) {
			return false;
		}

		Stream nstream(stream, descriptorLength);

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
