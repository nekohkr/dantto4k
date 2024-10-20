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

class MmtDescriptors {
public:
	MmtDescriptors();
	bool unpack(Stream& stream);
	std::list<std::shared_ptr<MmtDescriptorBase>> list;

private:
	std::unordered_map<uint16_t, std::function<std::shared_ptr<MmtDescriptorBase>()>> mapDescriptorFactory;
};