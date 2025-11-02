#include "mmtDescriptorFactory.h"
#include <functional>
#include <unordered_map>

#include "eventPackageDescriptor.h"
#include "mhAudioComponentDescriptor.h"
#include "mhCaContractInformation.h"
#include "mhContentDescriptor.h"
#include "mhDataComponentDescriptor.h"
#include "mhEventGroupDescriptor.h"
#include "mhExtendedEventDescriptor.h"
#include "mhLinkageDescriptor.h"
#include "mhLogoTransmissionDescriptor.h"
#include "mhParentalRatingDescriptor.h"
#include "mhSeriesDescriptor.h"
#include "mhServiceDescriptor.h"
#include "mhShortEventDescriptor.h"
#include "mhStreamIdentificationDescriptor.h"
#include "mpuExtendedTimestampDescriptor.h"
#include "mpuTimestampDescriptor.h"
#include "videoComponentDescriptor.h"
#include "contentCopyControlDescriptor.h"
#include "multimediaServiceInformationDescriptor.h"
#include "accessControlDescriptor.h"
#include "mhSiParameterDescriptor.h"
#include "relatedBroadcasterDescriptor.h"
#include "mhBroadcasterNameDescriptor.h"
#include "mhServiceListDescriptor.h"
#include <memory>

namespace MmtTlv {
	
static const std::unordered_map<uint16_t, std::function<std::shared_ptr<MmtDescriptorBase>()>> mapMmtDescriptor = {
	{ MhAudioComponentDescriptor::kDescriptorTag,				[] { return std::make_shared<MhAudioComponentDescriptor>(); } },
	{ MhContentDescriptor::kDescriptorTag,						[] { return std::make_shared<MhContentDescriptor>(); } },
	{ MhExtendedEventDescriptor::kDescriptorTag,				[] { return std::make_shared<MhExtendedEventDescriptor>(); } },
	{ MhServiceDescriptor::kDescriptorTag,						[] { return std::make_shared<MhServiceDescriptor>(); } },
	{ MhShortEventDescriptor::kDescriptorTag,					[] { return std::make_shared<MhShortEventDescriptor>(); } },
	{ MpuExtendedTimestampDescriptor::kDescriptorTag,			[] { return std::make_shared<MpuExtendedTimestampDescriptor>(); } },
	{ MpuTimestampDescriptor::kDescriptorTag,					[] { return std::make_shared<MpuTimestampDescriptor>(); } },
	{ VideoComponentDescriptor::kDescriptorTag,					[] { return std::make_shared<VideoComponentDescriptor>(); } },
	{ EventPackageDescriptor::kDescriptorTag,					[] { return std::make_shared<EventPackageDescriptor>(); } },
	{ MhCaContractInformation::kDescriptorTag,					[] { return std::make_shared<MhCaContractInformation>(); } },
	{ MhLinkageDescriptor::kDescriptorTag,						[] { return std::make_shared<MhLinkageDescriptor>(); } },
	{ MhLogoTransmissionDescriptor::kDescriptorTag,				[] { return std::make_shared<MhLogoTransmissionDescriptor>(); } },
	{ MhSeriesDescriptor::kDescriptorTag,						[] { return std::make_shared<MhSeriesDescriptor>(); } },
	{ MhEventGroupDescriptor::kDescriptorTag,					[] { return std::make_shared<MhEventGroupDescriptor>(); } },
	{ MhParentalRatingDescriptor::kDescriptorTag,				[] { return std::make_shared<MhParentalRatingDescriptor>(); } },
	{ MhStreamIdentificationDescriptor::kDescriptorTag,			[] { return std::make_shared<MhStreamIdentificationDescriptor>(); } },
	{ MhDataComponentDescriptor::kDescriptorTag,				[] { return std::make_shared<MhDataComponentDescriptor>(); } },
	{ ContentCopyControlDescriptor::kDescriptorTag,				[] { return std::make_shared<ContentCopyControlDescriptor>(); } },
	{ MultimediaServiceInformationDescriptor::kDescriptorTag,	[] { return std::make_shared<MultimediaServiceInformationDescriptor>(); } },
	{ AccessControlDescriptor::kDescriptorTag,					[] { return std::make_shared<AccessControlDescriptor>(); } },
	{ MhSiParameterDescriptor::kDescriptorTag,					[] { return std::make_shared<MhSiParameterDescriptor>(); } },
	{ RelatedBroadcasterDescriptor::kDescriptorTag,				[] { return std::make_shared<RelatedBroadcasterDescriptor>(); } },
	{ MhBroadcasterNameDescriptor::kDescriptorTag,				[] { return std::make_shared<MhBroadcasterNameDescriptor>(); } },
	{ MhServiceListDescriptor::kDescriptorTag,					[] { return std::make_shared<MhServiceListDescriptor>(); } },
};

std::shared_ptr<MmtDescriptorBase> MmtDescriptorFactory::create(uint16_t tag) {
	auto it = mapMmtDescriptor.find(tag);
	if (it == mapMmtDescriptor.end()) {
		return {};
	}

	return it->second();
}

bool MmtDescriptorFactory::isValidTag(uint16_t tag) {
	auto it = mapMmtDescriptor.find(tag);
	if (it == mapMmtDescriptor.end()) {
		return false;
	}

	return true;
}

}