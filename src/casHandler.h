#pragma once
#include <vector>
#include "mmt.h"

namespace MmtTlv {
	
class CasHandler {
public:
	virtual ~CasHandler() = default;

	virtual bool onEcm(const std::vector<uint8_t>& ecm) { return false; }
	virtual bool decrypt(MmtTlv::Mmt& mmt) { return false; }
		

};

}