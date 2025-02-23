#pragma once
#include "tlvDescriptorBase.h"
#include <list>
#include <memory>

namespace MmtTlv {

class TlvDescriptors {
public:
	bool unpack(Common::ReadStream& stream);

	std::list<std::shared_ptr<TlvDescriptorBase>> list;

};

}