#include "networkNameDescriptor.h"

namespace MmtTlv {

bool NetworkNameDescriptor::unpack(Common::ReadStream& stream)
{
    if (!TlvDescriptorTemplate::unpack(stream)) {
        return false;
    }

    int size = stream.leftBytes();
    networkName.resize(size);
    stream.read(networkName.data(), size);
    return true;
}

}