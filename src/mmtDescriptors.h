#pragma once
#include "mmtDescriptorBase.h"
#include <list>
#include <memory>

namespace MmtTlv {

class MmtDescriptors {
public:
	bool unpack(Common::ReadStream& stream);
	std::list<std::shared_ptr<MmtDescriptorBase>> list;

};

}