#include "mhExtendedEventDescriptor.h"

namespace MmtTlv {

bool MhExtendedEventDescriptor::unpack(Common::ReadStream& stream) {
    try {
        if (!MmtDescriptorTemplate::unpack(stream)) {
            return false;
        }

        Common::ReadStream nstream(stream, descriptorLength);

        uint8_t uint8 = nstream.get8U();
        descriptorNumber = (uint8 & 0b11110000) >> 4;
        lastDescriptorNumber = (uint8 & 0b00001111);
        nstream.read(language, 3);
        language[3] = '\0';

        lengthOfItems = nstream.getBe16U();
        Common::ReadStream nstream2(nstream, lengthOfItems);
        while (!nstream2.isEof()) {
            Entry entry;
            if (!entry.unpack(nstream2)) {
                return false;
            }

            entries.push_back(entry);
        }
        nstream.skip(lengthOfItems);


        textLength = nstream.getBe16U();
        if (textLength) {
            textChar.resize(textLength);
            nstream.read(textChar.data(), textLength);
        }

        stream.skip(descriptorLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

	return true;
}

bool MhExtendedEventDescriptor::Entry::unpack(Common::ReadStream& stream)
{
    try {
        itemDescriptionLength = stream.get8U();
        if (itemDescriptionLength > 0) {
            itemDescriptionChar.resize(itemDescriptionLength);
            stream.read(itemDescriptionChar.data(), itemDescriptionLength);
        }

        itemLength = stream.getBe16U();
        if (itemLength > 0) {
            itemChar.resize(itemLength);
            stream.read(itemChar.data(), itemLength);
        }
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

}