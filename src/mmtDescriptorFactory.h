#pragma once
#include "mmtDescriptorBase.h"
#include <list>
#include <memory>

namespace MmtTlv {

class MmtDescriptorFactory {
public:
	static std::unique_ptr<MmtDescriptorBase> create(uint16_t tag);
	static bool isValidTag(uint16_t tag);

};

}