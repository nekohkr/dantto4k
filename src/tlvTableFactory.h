#pragma once
#include <memory>

namespace MmtTlv {
class TlvTableBase;
class TlvTableFactory {
public:
	static std::shared_ptr<TlvTableBase> create(uint8_t id);
	static bool isValidId(uint8_t id);
	
};

}