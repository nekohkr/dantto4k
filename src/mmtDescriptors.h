#pragma once
#include "mmtDescriptor.h"
#include <unordered_map>
#include <list>
#include <functional>

#include "mhAudioComponentDescriptor.h"
#include "mhContentDescriptor.h"
#include "mhExtendedEventDescriptor.h"
#include "mhServiceDescriptor.h"
#include "mhShortEventDescriptor.h"
#include "mpuExtendedTimestampDescriptor.h"
#include "mpuTimestampDescriptor.h"
#include "videoComponentDescriptor.h"

#include "eventPackageDescriptor.h"
#include "mhCaContractInformation.h"
#include "mhLinkageDescriptor.h"
#include "mhLogoTransmissionDescriptor.h"
#include "mhSeriesDescriptor.h"
#include "mhEventGroupDescriptor.h"
#include "mhParentalRatingDescriptor.h"
#include "mhStreamIdentificationDescriptor.h"
#include "mhDataComponentDescriptor.h"

class MmtDescriptors {
public:
	MmtDescriptors();
	bool unpack(Stream& stream);
	std::list<std::shared_ptr<MmtDescriptorBase>> list;

private:
	std::unordered_map<uint16_t, std::function<std::shared_ptr<MmtDescriptorBase>()>> mapDescriptorFactory;
};