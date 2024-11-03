#pragma once
#include "mmtDescriptorBase.h"
#include <list>

namespace MmtTlv {

class MmtDescriptors {
public:
	bool unpack(Common::Stream& stream);
	std::list<std::shared_ptr<MmtDescriptorBase>> list;

};

}