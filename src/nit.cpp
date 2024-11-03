#include "nit.h"

namespace MmtTlv {

bool Nit::unpack(Common::Stream& stream)
{
    if (!TlvTableBase::unpack(stream)) {
        return false;
    }

    uint16_t uint16 = stream.getBe16U();
    sectionSyntaxIndicator = (uint16 & 0b1000000000000000) >> 15;
    sectionLength = uint16 & 0b0000111111111111;

    networkId = stream.getBe16U();

    uint8_t uint8 = stream.get8U();
    currentNextIndicator = uint8 & 1;
    sectionNumber = stream.get8U();
    lastSectionNumber = stream.get8U();

    uint16 = stream.getBe16U();
    networkDescriptorsLength = uint16 & 0b0000111111111111;

    {
        Common::Stream nstream(stream, networkDescriptorsLength);
        descriptors.unpack(nstream);
        stream.skip(networkDescriptorsLength);
    }

    uint16 = stream.getBe16U();
    tlvStreamLoopLength = uint16 & 0b0000111111111111;

    Common::Stream nstream(stream, tlvStreamLoopLength);
    while (!nstream.isEof()) {
        Entry entry;
        entry.unpack(nstream);
        entries.push_back(entry);
    }
    stream.skip(tlvStreamLoopLength);
    return true;
}

bool Nit::Entry::unpack(Common::Stream& stream)
{
    tlvStreamId = stream.getBe16U();
    originalNetworkId = stream.getBe16U();

    uint16_t uint16 = stream.getBe16U();
    tlvStreamDescriptorsLength = uint16 & 0b0000111111111111;

    Common::Stream nstream(stream, tlvStreamDescriptorsLength);
    descriptors.unpack(nstream);
    stream.skip(tlvStreamDescriptorsLength);
    return true;
}

}
