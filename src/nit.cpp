#include "nit.h"

namespace MmtTlv {

bool Nit::unpack(Common::ReadStream& stream)
{
    try {
        if (!TlvTableBase::unpack(stream)) {
            return false;
        }

        uint16_t uint16 = stream.getBe16U();
        sectionSyntaxIndicator = (uint16 & 0b1000000000000000) >> 15;
        sectionLength = uint16 & 0b0000111111111111;

        networkId = stream.getBe16U();

        uint8_t uint8 = stream.get8U();
        versionNumber = (uint8 & 0b00111110) >> 1;
        currentNextIndicator = uint8 & 1;
        sectionNumber = stream.get8U();
        lastSectionNumber = stream.get8U();

        uint16 = stream.getBe16U();
        networkDescriptorsLength = uint16 & 0b0000111111111111;

        {
            Common::ReadStream nstream(stream, networkDescriptorsLength);
            if (!descriptors.unpack(nstream)) {
                return false;
            }
            stream.skip(networkDescriptorsLength);
        }

        uint16 = stream.getBe16U();
        tlvStreamLoopLength = uint16 & 0b0000111111111111;

        Common::ReadStream nstream(stream, tlvStreamLoopLength);
        while (!nstream.isEof()) {
            Entry entry;
            if (!entry.unpack(nstream)) {
                return false;
            }
            entries.push_back(entry);
        }
        stream.skip(tlvStreamLoopLength);
	}
	catch (const std::out_of_range&) {
		return false;
	}

    return true;
}

bool Nit::Entry::unpack(Common::ReadStream& stream)
{
    try {
        tlvStreamId = stream.getBe16U();
        originalNetworkId = stream.getBe16U();

        uint16_t uint16 = stream.getBe16U();
        tlvStreamDescriptorsLength = uint16 & 0b0000111111111111;

        Common::ReadStream nstream(stream, tlvStreamDescriptorsLength);
        if (!descriptors.unpack(nstream)) {
            return false;
        }
        stream.skip(tlvStreamDescriptorsLength);
	}
	catch (const std::out_of_range&) {
		return false;
	}

    return true;
}

}
