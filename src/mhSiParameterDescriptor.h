#pragma once
#include "mmtDescriptorBase.h"
#include <list>

namespace MmtTlv {

class MhSiParameterDescriptor
	: public MmtDescriptorTemplate<0x8017> {
public:
	bool unpack(Common::Stream& stream) override;

	class Entry {
	public:
		bool unpack(Common::Stream& stream);

		uint8_t tableId;
		uint8_t tableDescriptionLength;
		std::vector<uint8_t> tableDescriptionByte;
	};

	uint8_t parameterVersion;
	uint16_t updateTime;
	std::list<Entry> entries;

};

}