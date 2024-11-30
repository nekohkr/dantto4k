#include "networkNameDescriptor.h"

namespace MmtTlv {

bool NetworkNameDescriptor::unpack(Common::ReadStream& stream)
{
    try {
        if (!TlvDescriptorTemplate::unpack(stream)) {
            return false;
        }

        size_t size = stream.leftBytes();
        networkName.resize(size);
        stream.read(networkName.data(), size);
	}
	catch (const std::out_of_range&) {
		return false;
	}

    return true;
}

}