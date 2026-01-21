#include "indexItem.h"

namespace MmtTlv {

bool IndexItem::Item::unpack(Common::ReadStream& stream) {
	try {
		itemId = stream.getBe32U();
		itemSize = stream.getBe32U();
		itemVersion = stream.get8U();

        uint8_t fileNameLength = stream.get8U();
        fileName.resize(fileNameLength);
        stream.read(fileName.data(), fileNameLength);

        uint8_t uint8 = stream.get8U();
        checksumFlag = (uint8 & 0b10000000) >> 7;
        if (checksumFlag) {
            itemChecksum = stream.getBe32U();
        }

        uint8_t itemTypeLength = stream.get8U();
        itemType.resize(itemTypeLength);
        stream.read(itemType.data(), itemTypeLength);

        compressionType = stream.get8U();
        if (compressionType != 0xFF) {
            originalSize = stream.getBe32U();
        }
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

bool IndexItem::unpack(Common::ReadStream& stream) {
    try {
        uint16_t numOfItems = stream.getBe16U();
        for (int i = 0; i < numOfItems; i++) {
            Item item;
            if (!item.unpack(stream)) {
                return false;
            }
            items.push_back(item);
        }
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

}
