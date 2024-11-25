#include "remoteControlKeyDescriptor.h"

namespace MmtTlv {

bool RemoteControlKeyDescriptor::unpack(Common::ReadStream& stream)
{
    if (!TlvDescriptorTemplate::unpack(stream)) {
        return false;
    }

    numOfRemoteControlKeyId = stream.get8U();
    for (int i = 0; i < numOfRemoteControlKeyId; i++) {
        Entry item;
        item.unpack(stream);
        entries.push_back(item);
    }

    return true;
}

bool RemoteControlKeyDescriptor::Entry::unpack(Common::ReadStream& stream)
{
    remoteControlKeyId = stream.get8U();
    serviceId = stream.getBe16U();
    return true;
}

}