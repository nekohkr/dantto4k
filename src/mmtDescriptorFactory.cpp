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
#include "mhApplicationDescriptor.h"
#include "mhTransportProtocolDescriptor.h"
#include "mhSimpleApplicationLocationDescriptor.h"
#include "mhApplicationBoundaryAndPermissionDescriptor.h"
#include "mhAutostartPriorityDescriptor.h"
#include "mhCacheControlInfoDescriptor.h"
#include "mhRandomizedLatencyDescriptor.h"

#include <memory>

namespace MmtTlv {
	
static const std::unordered_map<uint16_t, std::function<std::unique_ptr<MmtDescriptorBase>()>> mapMmtDescriptor = {
	{ MhAudioComponentDescriptor::kDescriptorTag,				[] { return std::make_unique<MhAudioComponentDescriptor>(); } },
	{ MhContentDescriptor::kDescriptorTag,						[] { return std::make_unique<MhContentDescriptor>(); } },
	{ MhExtendedEventDescriptor::kDescriptorTag,				[] { return std::make_unique<MhExtendedEventDescriptor>(); } },
	{ MhServiceDescriptor::kDescriptorTag,						[] { return std::make_unique<MhServiceDescriptor>(); } },
	{ MhShortEventDescriptor::kDescriptorTag,					[] { return std::make_unique<MhShortEventDescriptor>(); } },
	{ MpuExtendedTimestampDescriptor::kDescriptorTag,			[] { return std::make_unique<MpuExtendedTimestampDescriptor>(); } },
	{ MpuTimestampDescriptor::kDescriptorTag,					[] { return std::make_unique<MpuTimestampDescriptor>(); } },
	{ VideoComponentDescriptor::kDescriptorTag,					[] { return std::make_unique<VideoComponentDescriptor>(); } },
	{ EventPackageDescriptor::kDescriptorTag,					[] { return std::make_unique<EventPackageDescriptor>(); } },
	{ MhCaContractInformation::kDescriptorTag,					[] { return std::make_unique<MhCaContractInformation>(); } },
	{ MhLinkageDescriptor::kDescriptorTag,						[] { return std::make_unique<MhLinkageDescriptor>(); } },
	{ MhLogoTransmissionDescriptor::kDescriptorTag,				[] { return std::make_unique<MhLogoTransmissionDescriptor>(); } },
	{ MhSeriesDescriptor::kDescriptorTag,						[] { return std::make_unique<MhSeriesDescriptor>(); } },
	{ MhEventGroupDescriptor::kDescriptorTag,					[] { return std::make_unique<MhEventGroupDescriptor>(); } },
	{ MhParentalRatingDescriptor::kDescriptorTag,				[] { return std::make_unique<MhParentalRatingDescriptor>(); } },
	{ MhStreamIdentificationDescriptor::kDescriptorTag,			[] { return std::make_unique<MhStreamIdentificationDescriptor>(); } },
	{ MhDataComponentDescriptor::kDescriptorTag,				[] { return std::make_unique<MhDataComponentDescriptor>(); } },
	{ ContentCopyControlDescriptor::kDescriptorTag,				[] { return std::make_unique<ContentCopyControlDescriptor>(); } },
	{ MultimediaServiceInformationDescriptor::kDescriptorTag,	[] { return std::make_unique<MultimediaServiceInformationDescriptor>(); } },
	{ AccessControlDescriptor::kDescriptorTag,					[] { return std::make_unique<AccessControlDescriptor>(); } },
	{ MhSiParameterDescriptor::kDescriptorTag,					[] { return std::make_unique<MhSiParameterDescriptor>(); } },
	{ RelatedBroadcasterDescriptor::kDescriptorTag,				[] { return std::make_unique<RelatedBroadcasterDescriptor>(); } },
	{ MhBroadcasterNameDescriptor::kDescriptorTag,				[] { return std::make_unique<MhBroadcasterNameDescriptor>(); } },
	{ MhServiceListDescriptor::kDescriptorTag,					[] { return std::make_unique<MhServiceListDescriptor>(); } },

    { MhApplicationDescriptor::kDescriptorTag,					[] { return std::make_unique<MhApplicationDescriptor>(); } },
    { MhTransportProtocolDescriptor::kDescriptorTag,			[] { return std::make_unique<MhTransportProtocolDescriptor>(); } },
    { MhSimpleApplicationLocationDescriptor::kDescriptorTag,	[] { return std::make_unique<MhSimpleApplicationLocationDescriptor>(); } },
    { MhApplicationBoundaryAndPermissionDescriptor::kDescriptorTag, [] { return std::make_unique<MhApplicationBoundaryAndPermissionDescriptor>(); } },
    { MhAutostartPriorityDescriptor::kDescriptorTag,			[] { return std::make_unique<MhAutostartPriorityDescriptor>(); } },
	{ MhCacheControlInfoDescriptor::kDescriptorTag,				[] { return std::make_unique<MhCacheControlInfoDescriptor>(); } },
	{ MhRandomizedLatencyDescriptor::kDescriptorTag,				[] { return std::make_unique<MhRandomizedLatencyDescriptor>(); } },
};

std::unique_ptr<MmtDescriptorBase> MmtDescriptorFactory::create(uint16_t tag) {
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