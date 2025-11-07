#include "networkNameDescriptor.h"

namespace MmtTlv {

bool NetworkNameDescriptor::unpack(Common::ReadStream& stream)
{
    try {
        if (!TlvDescriptorTemplate::unpack(stream)) {
            return false;
        }

        Common::ReadStream nstream(stream, descriptorLength);

        size_t size = nstream.leftBytes();
        networkName.resize(size);
        nstream.read(networkName.data(), size);

        stream.skip(descriptorLength);
	}
	catch (const std::out_of_range&) {
		return false;
	}

    return true;
}

}