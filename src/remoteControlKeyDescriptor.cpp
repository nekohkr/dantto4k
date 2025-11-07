#include "remoteControlKeyDescriptor.h"

namespace MmtTlv {

bool RemoteControlKeyDescriptor::unpack(Common::ReadStream& stream)
{
    try {
        if (!TlvDescriptorTemplate::unpack(stream)) {
            return false;
        }

        Common::ReadStream nstream(stream, descriptorLength);

        numOfRemoteControlKeyId = nstream.get8U();
        for (int i = 0; i < numOfRemoteControlKeyId; i++) {
            Entry item;
            if (!item.unpack(nstream)) {
                return false;
            }
            entries.push_back(item);
        }

        stream.skip(descriptorLength);
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
    reserved = stream.getBe16U();
    return true;
}

}