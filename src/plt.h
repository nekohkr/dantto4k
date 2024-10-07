#pragma once
#include "mmtTable.h"
#include <list>
#include "mmtGeneralLocationInfo.h"

class PltItem {
public:
	bool unpack(Stream& stream);

	uint8_t mmtPackageIdLength;
	std::vector<uint8_t> mmtPackageIdByte;
	MmtGeneralLocationInfo locationInfos;
};

class Plt : public MmtTable {
public:
	bool unpack(Stream& stream);

	uint8_t version;
	uint16_t length;
	uint8_t numOfPackage;
	std::list<PltItem> items;
};