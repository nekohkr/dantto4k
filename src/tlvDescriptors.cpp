#include "tlvDescriptors.h"
#include "tlvDescriptorFactory.h"

namespace MmtTlv {

bool TlvDescriptors::unpack(Common::ReadStream& stream)
{
	list.clear();
	while (!stream.isEof()) {
		uint8_t descriptorTag = stream.peek8U();
		auto it = TlvDescriptorFactory::create(descriptorTag);
		if (it == nullptr) {
			TlvDescriptorBase base;
			if (!base.unpack(stream)) {
				return false;
			}
			stream.skip(base.getDescriptorLength());
		}
		else {
 			if (!it->unpack(stream)) {
				return false;
			}
			list.push_back(std::move(it));
		}
	}
	return true;
}


}