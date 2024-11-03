#pragma once
#include "mpuDataProcessorBase.h"

namespace MmtTlv {

class MpuDataProcessorBase;
class MpuDataProcessorFactory {
public:
	static std::shared_ptr<MpuDataProcessorBase> create(uint32_t tag);

};

}