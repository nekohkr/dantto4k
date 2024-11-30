#include "relatedBroadcasterDescriptor.h"

namespace MmtTlv {

bool RelatedBroadcasterDescriptor::unpack(Common::ReadStream& stream)
{
	try {
		if (!MmtDescriptorTemplate::unpack(stream)) {
			return false;
		}

        Common::ReadStream nstream(stream, descriptorLength);

		uint8_t uint8 = nstream.get8U();
		numOfBroadcasterId = (uint8 & 0b11110000) >> 4;
        numOfAffiliationId = uint8 & 0b00001111;

		uint8 = nstream.get8U();
        numOfOriginalNetworkId = (uint8 & 0b11110000) >> 4;

		for (int i = 0; i < numOfBroadcasterId; i++) {
			BroadcasterId broadcasterId;
			broadcasterId.unpack(nstream);
			broadcasterIds.push_back(broadcasterId);
		}
		
		for (int i = 0; i < numOfAffiliationId; i++) {
			affiliationIds.push_back(nstream.get8U());
		}
		
		for (int i = 0; i < numOfOriginalNetworkId; i++) {
			originalNetworkIds.push_back(nstream.getBe16U());
		}

        stream.skip(descriptorLength);
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

bool RelatedBroadcasterDescriptor::BroadcasterId::unpack(Common::ReadStream & stream)
{
	try {
		networkId = stream.getBe16U();
		broadcasterId = stream.get8U();
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

}