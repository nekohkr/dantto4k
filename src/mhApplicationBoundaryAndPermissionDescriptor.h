#pragma once
#include "mmtDescriptorBase.h"

namespace MmtTlv {

class MhApplicationBoundaryAndPermissionDescriptor
	: public MmtDescriptorTemplate<0x802C> {
public:
	bool unpack(Common::ReadStream& stream) override;

	class Entry {
    public:
        bool unpack(Common::ReadStream& stream);

        uint8_t permissionBitmapCount;
        std::vector<uint16_t> permissionBitmap;
        uint16_t managedUrlCount;
        std::vector<std::string> managedUrl;
	};

	std::vector<Entry> entires;

};

}