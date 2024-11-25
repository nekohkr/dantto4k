#pragma once
#include "stream.h"

namespace MmtTlv {
	
class TlvDescriptorBase {
public:
	virtual ~TlvDescriptorBase() = default;

	virtual bool unpack(Common::ReadStream& stream) {
		try {
			descriptorTag = stream.get8U();
			descriptorLength = stream.get8U();

			if (stream.leftBytes() < descriptorLength) {
				return false;
			}
		}
		catch (const std::out_of_range&) {
			return false;
		}

		return true;
	}

	uint8_t getDescriptorTag() const { return descriptorTag; };
	uint8_t getDescriptorLength() const { return descriptorLength; };

protected:
	uint8_t descriptorTag;
	uint8_t descriptorLength;
};

template<uint16_t descriptorTagValue>
class TlvDescriptorTemplate : public TlvDescriptorBase {
public:
	virtual ~TlvDescriptorTemplate() = default;

	static constexpr uint8_t kDescriptorTag = descriptorTagValue;

};

}