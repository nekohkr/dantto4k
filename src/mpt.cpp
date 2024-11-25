#include "mpt.h"

namespace MmtTlv {

bool Mpt::unpack(Common::ReadStream& stream)
{
	try {
		if (!MmtTableBase::unpack(stream)) {
			return false;
		}

		version = stream.get8U();
		length = stream.getBe16U();
		uint8_t uint8 = stream.get8U();
		reserved = (uint8 & 0b11111100) >> 2;
		mptMode = (uint8 & 0x00000011);

		mmtPackageIdLength = stream.get8U();
		if (stream.leftBytes() < mmtPackageIdLength) {
			return false;
		}

		mmtPackageIdByte.resize(mmtPackageIdLength);
		stream.read(mmtPackageIdByte.data(), mmtPackageIdLength);

		mptDescriptorsLength = stream.getBe16U();
		
		Common::ReadStream nstream(stream, mptDescriptorsLength);
		descriptors.unpack(nstream);
		stream.skip(mptDescriptorsLength);

		numberOfAssets = stream.get8U();
		for (int i = 0; i < numberOfAssets; i++) {
			Asset asset;
			asset.unpack(stream);
			assets.push_back(asset);
		}
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

bool Mpt::Asset::unpack(Common::ReadStream& stream)
{
	try {
		identifierType = stream.get8U();
		assetIdScheme = stream.getBe32U();
		assetIdLength = stream.get8U();

		assetIdByte.resize(assetIdLength);
		stream.read(assetIdByte.data(), assetIdLength);

		assetType = stream.getBe32U();
		uint8_t uint8 = stream.get8U();
		reserved = (uint8 & 0b11111110) >> 2;
		assetClockRelationFlag = (uint8 & 0x00000001);
		locationCount = stream.get8U();
		for (int i = 0; i < locationCount; i++) {
			MmtGeneralLocationInfo locationInfo;
			locationInfo.unpack(stream);
			locationInfos.push_back(locationInfo);
		}

		assetDescriptorsLength = stream.getBe16U();

		Common::ReadStream nstream(stream, assetDescriptorsLength);
		descriptors.unpack(nstream);
		stream.skip(assetDescriptorsLength);
	}
	catch (const std::out_of_range&) {
		return false;
	}
	return true;
}

}