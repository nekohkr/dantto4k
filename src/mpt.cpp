#include "mpt.h"

bool Mpt::unpack(Stream& stream)
{
	try {
		if (!MmtTable::unpack(stream)) {
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
		stream.read((char*)mmtPackageIdByte.data(), mmtPackageIdLength);

		mptDescriptorsLength = stream.getBe16U();
		mptDescriptorsByte.resize(mptDescriptorsLength);
		stream.read((char*)mptDescriptorsByte.data(), mptDescriptorsLength);

		numberOfAssets = stream.get8U();
		for (int i = 0; i < numberOfAssets; i++) {
			MptAsset asset;
			asset.unpack(stream);
			assets.push_back(asset);
		}
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

bool MptAsset::unpack(Stream& stream)
{
	try {
		identifierType = stream.get8U();
		assetIdScheme = stream.getBe32U();
		assetIdLength = stream.get8U();

		if (stream.leftBytes() < assetIdLength) {
			return false;
		}

		assetIdByte.resize(assetIdLength);
		stream.read((char*)assetIdByte.data(), assetIdLength);


		if (stream.leftBytes() < 4 + 1 + 1) {
			return false;
		}

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

		if (stream.leftBytes() < 2) {
			return false;
		}

		assetDescriptorsLength = stream.getBe16U();

		if (stream.leftBytes() < assetDescriptorsLength) {
			return false;
		}

		assetDescriptorsByte.resize(assetDescriptorsLength);
		stream.read((char*)assetDescriptorsByte.data(), assetDescriptorsLength);
	}
	catch (const std::out_of_range&) {
		return false;
	}
	return true;
}