#pragma once
#include "mmtTableBase.h"
#include "mmtGeneralLocationInfo.h"
#include "mmtDescriptors.h"

namespace MmtTlv {

// MMT Package Table
class Mpt : public MmtTableBase {
public:
	bool unpack(Common::ReadStream& stream);

	class Asset {
	public:
		bool unpack(Common::ReadStream& stream);

		uint8_t identifierType;
		uint32_t assetIdScheme;
		uint8_t assetIdLength;
		std::vector<uint8_t> assetIdByte;
		uint32_t assetType;
		uint8_t reserved;
		bool assetClockRelationFlag;
		uint8_t locationCount;
		std::vector<MmtGeneralLocationInfo> locationInfos;

		uint16_t assetDescriptorsLength;
		MmtDescriptors descriptors;
	};

	uint8_t version;
	uint16_t length;
	uint8_t reserved;
	uint8_t mptMode;
	uint8_t mmtPackageIdLength;
	std::vector<uint8_t> mmtPackageIdByte;
	uint16_t mptDescriptorsLength;
	MmtDescriptors descriptors;

	uint8_t numberOfAssets;
	std::list<Asset> assets;
};

}