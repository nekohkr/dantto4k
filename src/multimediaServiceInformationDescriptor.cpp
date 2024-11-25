#include "multimediaServiceInformationDescriptor.h"

namespace MmtTlv {

bool MultimediaServiceInformationDescriptor::unpack(Common::ReadStream& stream)
{
	try {
		if (!MmtDescriptorTemplate::unpack(stream)) {
			return false;
		}

        Common::ReadStream nstream(stream, descriptorLength);

        dataComponentId = nstream.getBe16U();
		if (dataComponentId == 0x0020) {
			componentTag = nstream.getBe16U();
			
			nstream.read(language, 3);
			language[3] = '\0';

			textLength = nstream.get8U();
			text.resize(textLength);
			nstream.read(text.data(), textLength);
		}
		
		if (dataComponentId == 0x0021) {
			uint8_t uint8 = nstream.get8U();
			associatedContentsFlag = (uint8 & 0b10000000) >> 7;
			reserved = uint8 & 0b01111111;
		}
		
		selectorLength = nstream.get8U();
		selectorByte.resize(selectorLength);
		nstream.read(selectorByte.data(), selectorLength);

        stream.skip(descriptorLength);
	}
	catch (const std::out_of_range&) {
		return false;
	}

	return true;
}

}