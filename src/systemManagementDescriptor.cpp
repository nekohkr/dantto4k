#include "systemManagementDescriptor.h"

namespace MmtTlv {

bool SystemManagementDescriptor::unpack(Common::ReadStream& stream)
{
    if (!TlvDescriptorTemplate::unpack(stream)) {
        return false;
    }

    Common::ReadStream nstream(stream, descriptorLength);
    
    systemManagementId = nstream.getBe16U();

    additionalIdentificationInfo.resize(nstream.leftBytes());
    nstream.read(additionalIdentificationInfo.data(), nstream.leftBytes());

    stream.skip(descriptorLength);
    return true;
}


}