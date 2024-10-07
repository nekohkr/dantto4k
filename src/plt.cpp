#include "plt.h"

bool Plt::unpack(Stream& stream)
{
	if (!MmtTable::unpack(stream)) {
		return false;
	}

	if (stream.leftBytes() < 1 + 2 + 1) {
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

	return false;
}

bool PltItem::unpack(Stream& stream)
{
	if (stream.leftBytes() < 1) {
		return false;
	}

	mmtPackageIdLength = stream.get8U();

	if (stream.leftBytes() < mmtPackageIdLength) {
		return false;
	}

	mmtPackageIdByte.resize(mmtPackageIdLength);
	stream.read((char*)mmtPackageIdByte.data(), mmtPackageIdLength);
	locationInfos.unpack(stream);
	return false;
}
