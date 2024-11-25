#include "mhEventGroupDescriptor.h"

namespace MmtTlv {

bool MhEventGroupDescriptor::unpack(Common::ReadStream& stream)
{
	try {
		if (!MmtDescriptorTemplate::unpack(stream)) {
			return false;
		}

		Common::ReadStream nstream(stream, descriptorLength);

		uint8_t uint8 = nstream.get8U();
		groupType = (uint8 & 0b11110000) >> 4;
		eventCount = uint8 & 0b00001111;

		for (int i = 0; i < eventCount; i++) {
			Event event;
			event.unpack(nstream);
			events.push_back(event);
		}

		if (groupType == 4 || groupType == 5) {
			while (!nstream.isEof()) {
				OtherNetworkEvent otherNetworkEvent;
				otherNetworkEvent.unpack(nstream);
				otherNetworkEvents.push_back(otherNetworkEvent);
			}
		}
		else {
			privateDataByte.resize(nstream.leftBytes());
			nstream.read(privateDataByte.data(), nstream.leftBytes());
		}

		stream.skip(descriptorLength);
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

bool MhEventGroupDescriptor::Event::unpack(Common::ReadStream& stream)
{
	serviceId = stream.getBe16U();
	eventId = stream.getBe16U();
	return true;
}

bool MhEventGroupDescriptor::OtherNetworkEvent::unpack(Common::ReadStream& stream)
{
	originalNetworkId = stream.getBe16U();
	tlvStreamId = stream.getBe16U();
	serviceId = stream.getBe16U();
	eventId = stream.getBe16U();
	return true;
}

}