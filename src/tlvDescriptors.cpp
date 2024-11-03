#include "tlvDescriptors.h"
#include "tlvDescriptorFactory.h"

namespace MmtTlv {

bool TlvDescriptors::unpack(Common::Stream& stream)
{
	list.clear();
	while (!stream.isEof()) {
		uint16_t descriptorTag = stream.peekBe16U();
		auto it = TlvDescriptorFactory::create(descriptorTag);
		if (it == nullptr) {
			TlvDescriptorBase base;
			base.unpack(stream);
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