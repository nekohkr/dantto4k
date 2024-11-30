#pragma once
#include "mmtDescriptorBase.h"
#include <list>

namespace MmtTlv {

class RelatedBroadcasterDescriptor
	: public MmtDescriptorTemplate<0x803E> {
public:
	bool unpack(Common::ReadStream& stream) override;
	
	class BroadcasterId {
	public:
		bool unpack(Common::ReadStream& stream);

		uint16_t networkId;
		uint8_t broadcasterId;
	};

	uint8_t numOfBroadcasterId;
	uint8_t numOfAffiliationId;
	uint8_t numOfOriginalNetworkId;

	std::list<BroadcasterId> broadcasterIds;
	std::list<uint8_t> affiliationIds;
	std::list<uint16_t> originalNetworkIds;

};

}