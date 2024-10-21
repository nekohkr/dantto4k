#include "mmtDescriptors.h"

MmtDescriptors::MmtDescriptors()
{
	mapDescriptorFactory[MhAudioComponentDescriptor::kDescriptorTag] =			[] { return std::make_shared<MhAudioComponentDescriptor>(); };
	mapDescriptorFactory[MhContentDescriptor::kDescriptorTag] =					[] { return std::make_shared<MhContentDescriptor>(); };
	mapDescriptorFactory[MhExtendedEventDescriptor::kDescriptorTag] =			[] { return std::make_shared<MhExtendedEventDescriptor>(); };
	mapDescriptorFactory[MhServiceDescriptor::kDescriptorTag] =					[] { return std::make_shared<MhServiceDescriptor>(); };
	mapDescriptorFactory[MhShortEventDescriptor::kDescriptorTag] =				[] { return std::make_shared<MhShortEventDescriptor>(); };
	mapDescriptorFactory[MpuExtendedTimestampDescriptor::kDescriptorTag] =		[] { return std::make_shared<MpuExtendedTimestampDescriptor>(); };
	mapDescriptorFactory[MpuTimestampDescriptor::kDescriptorTag] =				[] { return std::make_shared<MpuTimestampDescriptor>(); };
	mapDescriptorFactory[VideoComponentDescriptor::kDescriptorTag] =			[] { return std::make_shared<VideoComponentDescriptor>(); };

	mapDescriptorFactory[EventPackageDescriptor::kDescriptorTag] =				[] { return std::make_shared<EventPackageDescriptor>(); };
	mapDescriptorFactory[MhCaContractInformation::kDescriptorTag] =				[] { return std::make_shared<MhCaContractInformation>(); };
	mapDescriptorFactory[MhLinkageDescriptor::kDescriptorTag] =					[] { return std::make_shared<MhLinkageDescriptor>(); };
	mapDescriptorFactory[MhLogoTransmissionDescriptor::kDescriptorTag] =		[] { return std::make_shared<MhLogoTransmissionDescriptor>(); };
	mapDescriptorFactory[MhEventGroupDescriptor::kDescriptorTag] =				[] { return std::make_shared<MhEventGroupDescriptor>(); };
	mapDescriptorFactory[MhParentalRatingDescriptor::kDescriptorTag] =			[] { return std::make_shared<MhParentalRatingDescriptor>(); };
	mapDescriptorFactory[MhStreamIdentificationDescriptor::kDescriptorTag] =	[] { return std::make_shared<MhStreamIdentificationDescriptor>(); };
	mapDescriptorFactory[MhDataComponentDescriptor::kDescriptorTag] =			[] { return std::make_shared<MhDataComponentDescriptor>(); };
	
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
			stream.skip(base.getDescriptorLength());
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