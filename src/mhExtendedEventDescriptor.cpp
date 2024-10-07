#include "mhExtendedEventDescriptor.h"

bool MhExtendedEventDescriptor::unpack(Stream& stream)
{
    if (stream.leftBytes() < 3) {
        return false;
    }

    if (!MmtDescriptor::unpack(stream, true)) {
        return false;
    }

    uint8_t uint8 = stream.get8U();
    descriptorNumber = (uint8 & 0b11110000) >> 4;
    lastDescriptorNumber = (uint8 & 0b00001111);
    stream.read(language, 3);
    language[3] = '\0';

    lengthOfItems = stream.getBe16U();
    Stream nstream(stream, lengthOfItems);
    while(!nstream.isEOF()) {
        MhExtendedEventItem item;
        if (!item.unpack(nstream)) {
            return false;
        }

        items.push_back(item);
    }
    stream.skip(lengthOfItems);


    textLength = stream.getBe16U();
    if (textLength) {
        textChar.resize(textLength);
        stream.read(textChar.data(), textLength);
    }
	return true;
}

bool MhExtendedEventItem::unpack(Stream& stream)
{
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
    return true;
}
