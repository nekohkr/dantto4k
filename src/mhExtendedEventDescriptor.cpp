#include "mhExtendedEventDescriptor.h"

namespace MmtTlv {

bool MhExtendedEventDescriptor::unpack(Common::ReadStream& stream)
{
    try {
        if (!MmtDescriptorTemplate::unpack(stream)) {
            return false;
        }

        uint8_t uint8 = stream.get8U();
        descriptorNumber = (uint8 & 0b11110000) >> 4;
        lastDescriptorNumber = (uint8 & 0b00001111);
        stream.read(language, 3);
        language[3] = '\0';

        lengthOfItems = stream.getBe16U();
        Common::ReadStream nstream(stream, lengthOfItems);
        while (!nstream.isEof()) {
            Entry entry;
            if (!entry.unpack(nstream)) {
                return false;
            }

            entries.push_back(entry);
        }
        stream.skip(lengthOfItems);


        textLength = stream.getBe16U();
        if (textLength) {
            textChar.resize(textLength);
            stream.read(textChar.data(), textLength);
        }
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