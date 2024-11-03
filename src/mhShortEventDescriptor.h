#pragma once
#include "mmtDescriptorBase.h"

namespace MmtTlv {

class MhShortEventDescriptor
	: public MmtDescriptorTemplate<0xF001, true> {
public:
	bool unpack(Common::Stream& stream) override;

	char language[4];
	std::string eventName;
	std::string text;

};

}