#pragma once
#include "stream.h"
#include <map>

class MmtDescriptorBase {
public:
	virtual ~MmtDescriptorBase() = default;

	virtual bool unpack(Stream& stream) {
		try {
			descriptorTag = stream.getBe16U();
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

	uint16_t getDescriptorTag() const { return descriptorTag; };
	uint16_t getDescriptorLength() const { return descriptorLength; };

protected:
	uint16_t descriptorTag;
	uint16_t descriptorLength;
};

template<uint16_t descriptorTagValue, bool is16BitLength = false>
class MmtDescriptor : public MmtDescriptorBase {
public:
	virtual ~MmtDescriptor() = default;

	static constexpr uint16_t kDescriptorTag = descriptorTagValue;
	static constexpr uint16_t kIs16BitLength = is16BitLength;

	virtual bool unpack(Stream& stream) {
		try {
			descriptorTag = stream.getBe16U();
			if (kIs16BitLength) {
				descriptorLength = stream.getBe16U();
			}
			else {
				descriptorLength = stream.get8U();
			}

			if (stream.leftBytes() < descriptorLength) {
				return false;
			}
		}
		catch (const std::out_of_range&) {
			return false;
		}

		return true;
	}


};

