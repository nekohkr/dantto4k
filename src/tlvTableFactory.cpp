#include "tlvTableFactory.h"
#include "nit.h"
#include <unordered_map>
#include <functional>

namespace MmtTlv {

static const std::unordered_map<uint8_t, std::function<std::shared_ptr<TlvTableBase>()>> mapTlvTableFactory = {
	{ TlvTableId::Nit,		[] { return std::make_shared<Nit>(); } },
};

std::shared_ptr<TlvTableBase> TlvTableFactory::create(uint8_t id) {
	auto it = mapTlvTableFactory.find(id);
	if (it == mapTlvTableFactory.end()) {
		return {};
	}

	return it->second();
}

bool TlvTableFactory::isValidId(uint8_t id)
{
	auto it = mapTlvTableFactory.find(id);
	if (it == mapTlvTableFactory.end()) {
		return false;
	}

	return true;
}

}