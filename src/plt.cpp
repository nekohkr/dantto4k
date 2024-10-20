#include "plt.h"

bool Plt::unpack(Stream& stream)
{
	try {
		if (!MmtTable::unpack(stream)) {
			return false;
		}

		version = stream.get8U();
		length = stream.getBe16U();
		numOfPackage = stream.get8U();

		for (int i = 0; i < numOfPackage; i++) {
			PltItem item;
			item.unpack(stream);
			items.push_back(item);
		}
	}
	catch (const std::out_of_range&) {
		return false;
	}
	return true;
}

bool PltItem::unpack(Stream& stream)
{
	try {
		mmtPackageIdLength = stream.get8U();

		if (stream.leftBytes() < mmtPackageIdLength) {
			return false;
		}

		mmtPackageIdByte.resize(mmtPackageIdLength);
		stream.read(mmtPackageIdByte.data(), mmtPackageIdLength);
		locationInfos.unpack(stream);
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}
