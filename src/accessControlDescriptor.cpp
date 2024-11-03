#include "accessControlDescriptor.h"

namespace MmtTlv {

bool AccessControlDescriptor::unpack(Common::Stream& stream)
{
    try {
        if (!MmtDescriptorTemplate::unpack(stream)) {
            return false;
        }

        Common::Stream nstream(stream, descriptorLength);

        caSystemId = nstream.getBe16U();
        locationInfo.unpack(nstream);

        privateData.resize(nstream.leftBytes());
        nstream.read(privateData.data(), nstream.leftBytes());

        stream.skip(descriptorLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

}