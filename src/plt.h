#pragma once
#include <list>
#include "mmtTableBase.h"
#include "mmtGeneralLocationInfo.h"

namespace MmtTlv {

// Package List Table
class Plt : public MmtTableBase {
public:
	bool unpack(Common::Stream& stream);

	class Entry {
	public:
		bool unpack(Common::Stream& stream);

		uint8_t mmtPackageIdLength;
		std::vector<uint8_t> mmtPackageIdByte;
		MmtGeneralLocationInfo locationInfos;
	};

	uint8_t version;
	uint16_t length;
	uint8_t numOfPackage;
	std::list<Entry> entries;
};

}