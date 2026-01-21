#include "mhTransportProtocolDescriptor.h"

namespace MmtTlv {

bool MhTransportProtocolDescriptor::unpack(Common::ReadStream& stream) {
    try {
        if (!MmtDescriptorTemplate::unpack(stream)) {
            return false;
        }

        Common::ReadStream nstream(stream, descriptorLength);

        protocolId = nstream.getBe16U();
        transportProtocolLabel = nstream.get8U();
        selector.resize(nstream.leftBytes());
        nstream.read(selector.data(), nstream.leftBytes());

        stream.skip(descriptorLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

}