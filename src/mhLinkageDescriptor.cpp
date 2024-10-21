#include "mhLinkageDescriptor.h"

bool MhLinkageDescriptor::unpack(Stream& stream)
{
    try {
        if (!MmtDescriptor::unpack(stream)) {
            return false;
        }

        Stream nstream(stream, descriptorLength);

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
