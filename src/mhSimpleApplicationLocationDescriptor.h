#pragma once
#include "mmtDescriptorBase.h"
#include <vector>

namespace MmtTlv {

class MhSimpleApplicationLocationDescriptor
	: public MmtDescriptorTemplate<0x802B> {
public:
	bool unpack(Common::ReadStream& stream) override;

	std::string initialPath;

};

}