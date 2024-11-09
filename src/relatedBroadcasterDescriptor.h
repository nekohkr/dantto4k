#pragma once
#include "mmtDescriptorBase.h"
#include <list>

namespace MmtTlv {

class RelatedBroadcasterDescriptor
	: public MmtDescriptorTemplate<0x803E> {
public:
	bool unpack(Common::Stream& stream) override;
	
	class BroadcasterId {
	public:
		bool unpack(Common::Stream& stream);

		uint16_t networkId;
		uint8_t broadcasterId;
	};

	uint8_t numOfBroadcasterId;
	uint8_t numOfAffiliationId;
	uint8_t numOfOriginalNetworkId;

	std::list<BroadcasterId> broadcasterIds;
	std::list<uint16_t> affiliationIds;
	std::list<uint16_t> originalNetworkIds;

};

}