#include "mmtDescriptors.h"

MmtDescriptors::MmtDescriptors()
{
	mapDescriptorFactory[MhAudioComponentDescriptor::kDescriptorTag] =		[] { return std::make_shared<MhAudioComponentDescriptor>(); };
	mapDescriptorFactory[MhContentDescriptor::kDescriptorTag] =				[] { return std::make_shared<MhContentDescriptor>(); };
	mapDescriptorFactory[MhExtendedEventDescriptor::kDescriptorTag] =		[] { return std::make_shared<MhExtendedEventDescriptor>(); };
	mapDescriptorFactory[MhServiceDescriptor::kDescriptorTag] =				[] { return std::make_shared<MhServiceDescriptor>(); };
	mapDescriptorFactory[MhShortEventDescriptor::kDescriptorTag] =			[] { return std::make_shared<MhShortEventDescriptor>(); };
	mapDescriptorFactory[MpuExtendedTimestampDescriptor::kDescriptorTag] =	[] { return std::make_shared<MpuExtendedTimestampDescriptor>(); };
	mapDescriptorFactory[MpuTimestampDescriptor::kDescriptorTag] =			[] { return std::make_shared<MpuTimestampDescriptor>(); };
	mapDescriptorFactory[VideoComponentDescriptor::kDescriptorTag] =			[] { return std::make_shared<VideoComponentDescriptor>(); };
}

bool MmtDescriptors::unpack(Stream& stream)
{
	list.clear();
	while (!stream.isEOF()) {
		uint16_t descriptorTag = stream.peekBe16U();

		auto it = mapDescriptorFactory.find(descriptorTag);
		if (it == mapDescriptorFactory.end()) {
			MmtDescriptorBase base;
			base.unpack(stream);
		}
		else {
			auto descriptorFactory = it->second();
 			if (!descriptorFactory->unpack(stream)) {
				return false;
			}
			list.push_back(descriptorFactory);
		}
	}
	return true;
}