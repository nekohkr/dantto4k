#include "plt.h"

namespace MmtTlv {

bool Plt::unpack(Common::Stream& stream)
{
	try {
		if (!MmtTableBase::unpack(stream)) {
			return false;
		}

		version = stream.get8U();
		length = stream.getBe16U();
		numOfPackage = stream.get8U();

		for (int i = 0; i < numOfPackage; i++) {
			Entry item;
			item.unpack(stream);
			entries.push_back(item);
		}
	}
	catch (const std::out_of_range&) {
		return false;
	}
	return true;
}

bool Plt::Entry::unpack(Common::Stream& stream)
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

}