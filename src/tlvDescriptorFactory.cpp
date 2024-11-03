#include "tlvDescriptorFactory.h"
#include "serviceListDescriptor.h"
#include "remoteControlKeyDescriptor.h"
#include "networkNameDescriptor.h"
#include <unordered_map>
#include <functional>

namespace MmtTlv {

static const std::unordered_map<uint8_t, std::function<std::shared_ptr<TlvDescriptorBase>()>> mapTlvDescriptor = {
	{ ServiceListDescriptor::kDescriptorTag,		[] { return std::make_shared<ServiceListDescriptor>(); } },
	{ RemoteControlKeyDescriptor::kDescriptorTag,	[] { return std::make_shared<RemoteControlKeyDescriptor>(); } },
	{ NetworkNameDescriptor::kDescriptorTag,		[] { return std::make_shared<NetworkNameDescriptor>(); } },
};

std::shared_ptr<TlvDescriptorBase> TlvDescriptorFactory::create(uint8_t tag) {
	auto it = mapTlvDescriptor.find(tag);
	if (it == mapTlvDescriptor.end()) {
		return {};
	}

	return it->second();
}

bool TlvDescriptorFactory::isValidTag(uint8_t tag)
{
	auto it = mapTlvDescriptor.find(tag);
	if (it == mapTlvDescriptor.end()) {
		return false;
	}

	return true;
}

}