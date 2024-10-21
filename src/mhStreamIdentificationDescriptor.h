#pragma once
#include "mmtDescriptor.h"

class MhStreamIdentificationDescriptor
	: public MmtDescriptor<0x8011> {
public:
	bool unpack(Stream& stream) override;

	uint16_t componentTag;

};