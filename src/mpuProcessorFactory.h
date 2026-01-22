#pragma once
#include "mpuProcessorBase.h"

namespace MmtTlv {

class MpuProcessorBase;
class MpuProcessorFactory {
public:
	static std::unique_ptr<MpuProcessorBase> create(uint32_t tag);

};

}