#pragma once
#include "mpuDataProcessorBase.h"

class MpuDataProcessorBase;
class MpuDataProcessorFactory {
public:
	static std::shared_ptr<MpuDataProcessorBase> create(uint32_t tag);

};