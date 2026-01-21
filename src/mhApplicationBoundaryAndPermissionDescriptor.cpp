#include "mhApplicationBoundaryAndPermissionDescriptor.h"

namespace MmtTlv {
 
bool MhApplicationBoundaryAndPermissionDescriptor::unpack(Common::ReadStream& stream) {
    try {
        if (!MmtDescriptorTemplate::unpack(stream)) {
            return false;
        }

        Common::ReadStream nstream(stream, descriptorLength);

        while (!nstream.isEof()) {
            Entry entry;
            if (!entry.unpack(nstream)) {
                return false;
            }
            entires.push_back(entry);
        }
        stream.skip(descriptorLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

bool MhApplicationBoundaryAndPermissionDescriptor::Entry::unpack(Common::ReadStream& stream) {
    try {
        permissionBitmapCount = stream.get8U();
        for (int i = 0; i < permissionBitmapCount; i++) {
            permissionBitmap.push_back(stream.getBe16U());
        }

        managedUrlCount = stream.get8U();
        for (int i = 0; i < managedUrlCount; i++) {
            uint8_t urlLength = stream.get8U();
            std::string url;
            url.resize(urlLength);
            stream.read(url.data(), urlLength);
            managedUrl.push_back(url);
        }
    }
    catch (const std::out_of_range&) {
        return false;
    }

    return true;
}

}