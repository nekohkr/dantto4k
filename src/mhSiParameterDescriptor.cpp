#include "mhSiParameterDescriptor.h"

namespace MmtTlv {

bool MhSiParameterDescriptor::unpack(Common::ReadStream& stream)
{
    try {
        if (!MmtDescriptorTemplate::unpack(stream)) {
            return false;
        }
        
        Common::ReadStream nstream(stream, descriptorLength);

        parameterVersion = nstream.get8U();
        updateTime = nstream.getBe16U();

        while (!nstream.isEof()) {
            Entry entry;
            if (!entry.unpack(nstream)) {
                return false;
            }

            entries.push_back(entry);
        }
        
        stream.skip(descriptorLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

	return true;
}

bool MhSiParameterDescriptor::Entry::unpack(Common::ReadStream& stream)
{
    try {
        tableId = stream.get8U();
        tableDescriptionLength = stream.get8U();

        tableDescriptionByte.resize(tableDescriptionLength);
        stream.read(tableDescriptionByte.data(), tableDescriptionLength);
    }
    catch (const std::out_of_range&) {
        return false;
    }

	return true;
}
}