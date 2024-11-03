#pragma once
#include "tlvDescriptorBase.h"

namespace MmtTlv {

class TlvDescriptorFactory {
public:
	static std::shared_ptr<TlvDescriptorBase> create(uint8_t tag);
	static bool isValidTag(uint8_t tag);
	
};

}