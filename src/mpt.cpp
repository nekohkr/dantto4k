#include "mpt.h"

bool Mpt::unpack(Stream& stream)
{
	if (!MmtTable::unpack(stream)) {
		return false;
	}

	if (stream.leftBytes() < 1 + 2 + 1 + 1 + 2 + 1) {
		return false;
	}

	version = stream.get8U();
	length = stream.getBe16U();
	uint8_t uint8 = stream.get8U();
	reserved = (uint8 & 0b11111100) >> 2;
	mptMode = (uint8 & 0x00000011);

	mmtPackageIdLength = stream.get8U();
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

	return true;
}

bool MptAsset::unpack(Stream& stream)
{
	if (stream.leftBytes() < 1 + 4 + 1 + 4 + 1 + 1 + 2) {
		return false;
	}

	identifierType = stream.get8U();
	assetIdScheme = stream.getBe32U();
	assetIdLength = stream.get8U();

	if (stream.leftBytes() < assetIdLength) {
		return false;
	}

	assetIdByte.resize(assetIdLength);
	stream.read((char*)assetIdByte.data(), assetIdLength);

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

	if (stream.leftBytes() < assetDescriptorsLength) {
		return false;
	}

	assetDescriptorsByte.resize(assetDescriptorsLength);
	stream.read((char*)assetDescriptorsByte.data(), assetDescriptorsLength);

	return true;
}