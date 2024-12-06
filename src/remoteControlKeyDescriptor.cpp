#include "remoteControlKeyDescriptor.h"

namespace MmtTlv {

bool RemoteControlKeyDescriptor::unpack(Common::ReadStream& stream)
{
    try {
        if (!TlvDescriptorTemplate::unpack(stream)) {
            return false;
        }

        numOfRemoteControlKeyId = stream.get8U();
        for (int i = 0; i < numOfRemoteControlKeyId; i++) {
            Entry item;
            if (!item.unpack(stream)) {
                return false;
            }
            entries.push_back(item);
        }
	}
	catch (const std::out_of_range&) {
		return false;
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