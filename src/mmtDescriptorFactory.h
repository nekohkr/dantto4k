#pragma once
#include "mmtDescriptorBase.h"
#include <list>

namespace MmtTlv {

class MmtDescriptorFactory {
public:
	static std::shared_ptr<MmtDescriptorBase> create(uint16_t tag);
	static bool isValidTag(uint16_t tag);

};

}