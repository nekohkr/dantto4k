#pragma once
#include "mmtDescriptorBase.h"
#include <list>

namespace MmtTlv {

class MhServiceListDescriptor
	: public MmtDescriptorTemplate<0x800D> {
public:
	virtual ~MhServiceListDescriptor() {}
	bool unpack(Common::ReadStream& stream) override;

	class Entry {
	public:
		bool unpack(Common::ReadStream& stream);

		uint16_t serviceId;
		uint8_t serviceType;
	};

	std::list<Entry> entries;
};

}