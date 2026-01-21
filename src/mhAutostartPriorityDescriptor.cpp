#include "mhAutostartPriorityDescriptor.h"

namespace MmtTlv {

bool MhAutostartPriorityDescriptor::unpack(Common::ReadStream& stream) {
    try {
        if (!MmtDescriptorTemplate::unpack(stream)) {
            return false;
        }

        Common::ReadStream nstream(stream, descriptorLength);

        autostartPriority = nstream.get8U();

        stream.skip(descriptorLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

}