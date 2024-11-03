#include "mhStreamIdentificationDescriptor.h"

namespace MmtTlv {

bool MhStreamIdentificationDescriptor::unpack(Common::Stream& stream)
{
    try {
        if (!MmtDescriptorTemplate::unpack(stream)) {
            return false;
        }

        Common::Stream nstream(stream, descriptorLength);

        componentTag = nstream.getBe16U();

        stream.skip(descriptorLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

}