#pragma once
#include "mfuDataProcessorBase.h"

namespace MmtTlv {

class MfuDataProcessorBase;
class MfuDataProcessorFactory {
public:
	static std::shared_ptr<MfuDataProcessorBase> create(uint32_t tag);

};

}