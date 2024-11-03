#pragma once
#include "mmtDescriptorBase.h"

namespace MmtTlv {

class MhStreamIdentificationDescriptor
	: public MmtDescriptorTemplate<0x8011> {
public:
	bool unpack(Common::Stream& stream) override;

	uint16_t componentTag;

};

}