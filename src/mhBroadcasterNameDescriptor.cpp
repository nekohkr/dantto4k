#include "mhBroadcasterNameDescriptor.h"

namespace MmtTlv {

bool MhBroadcasterNameDescriptor::unpack(Common::ReadStream& stream)
{
    try {
        if (!MmtDescriptorTemplate::unpack(stream)) {
            return false;
        }

        Common::ReadStream nstream(stream, descriptorLength);
        
        text.resize(nstream.leftBytes());
        nstream.read(text.data(), nstream.leftBytes());

        stream.skip(descriptorLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

}