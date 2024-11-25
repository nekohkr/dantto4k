#include "mmtDescriptors.h"
#include "mmtDescriptorFactory.h"

namespace MmtTlv {

bool MmtDescriptors::unpack(Common::ReadStream& stream)
{
	list.clear();
	while (!stream.isEof()) {
		uint16_t descriptorTag = stream.peekBe16U();
		auto it = MmtDescriptorFactory::create(descriptorTag);
		if (it == nullptr) {
			MmtDescriptorBase base;
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