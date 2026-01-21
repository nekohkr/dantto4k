#include "mhSimpleApplicationLocationDescriptor.h"

namespace MmtTlv {

bool MhSimpleApplicationLocationDescriptor::unpack(Common::ReadStream& stream) {
    try {
        if (!MmtDescriptorTemplate::unpack(stream)) {
            return false;
        }

        Common::ReadStream nstream(stream, descriptorLength);

        initialPath.resize(nstream.leftBytes());
        nstream.read(initialPath.data(), nstream.leftBytes());

        stream.skip(descriptorLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

}