#include "plt.h"

namespace MmtTlv {

bool Plt::unpack(Common::ReadStream& stream)
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
			if (!item.unpack(stream)) {
				return false;
			}
			entries.push_back(item);
		}
	}
	catch (const std::out_of_range&) {
		return false;
	}
	return true;
}

bool Plt::Entry::unpack(Common::ReadStream& stream)
{
	try {
		mmtPackageIdLength = stream.get8U();

		if (stream.leftBytes() < mmtPackageIdLength) {
			return false;
		}

		mmtPackageIdByte.resize(mmtPackageIdLength);
		stream.read(mmtPackageIdByte.data(), mmtPackageIdLength);
		if (!locationInfos.unpack(stream)) {
			return false;
		}
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

}