#include "serviceListDescriptor.h"

namespace MmtTlv {

bool ServiceListDescriptor::unpack(Common::Stream& stream)
{
    if (!TlvDescriptorTemplate::unpack(stream)) {
        return false;
    }

    Common::Stream nstream(stream, descriptorLength);
    while (!nstream.isEof()) {
        Entry item;
        item.unpack(nstream);
        services.push_back(item);
    }
    stream.skip(descriptorLength);
    return true;
}

bool ServiceListDescriptor::Entry::unpack(Common::Stream& stream)
{
    serviceId = stream.getBe16U();
    serviceType = stream.get8U();
    return true;
}

}