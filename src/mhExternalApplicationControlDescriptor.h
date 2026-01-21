#pragma once
#include "mmtDescriptorBase.h"
#include <vector>

namespace MmtTlv {

class MhExternalApplicationControlDescriptor
	: public MmtDescriptorTemplate<0x803A> {
public:
	bool unpack(Common::ReadStream& stream) override;

};

}