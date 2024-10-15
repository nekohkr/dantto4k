#pragma once
#include "mmtDescriptor.h"

class MhShortEventDescriptor
	: public MmtDescriptor<0xF001, true> {
public:
	bool unpack(Stream& stream) override;

	char language[4];
	std::string eventName;
	std::string text;

};