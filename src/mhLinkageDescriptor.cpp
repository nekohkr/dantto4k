#include "mhLinkageDescriptor.h"

namespace MmtTlv {

bool MhLinkageDescriptor::unpack(Common::ReadStream& stream)
{
    try {
        if (!MmtDescriptorTemplate::unpack(stream)) {
            return false;
        }

        Common::ReadStream nstream(stream, descriptorLength);

        tlvStreamId = nstream.getBe16U();
        originalNetworkId = nstream.getBe16U();
        serviceId = nstream.getBe16U();
        linkageType = nstream.get8U();

        privateDataByte.resize(nstream.leftBytes());
        nstream.read(privateDataByte.data(), nstream.leftBytes());

        stream.skip(descriptorLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

}