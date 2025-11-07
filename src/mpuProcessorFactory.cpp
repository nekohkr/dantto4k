#include "mpuProcessorFactory.h"
#include "mpuApplicationProcessor.h"
#include "mpuVideoProcessor.h"
#include "mpuAudioProcessor.h"
#include "mpuSubtitleProcessor.h"

namespace MmtTlv {

std::shared_ptr<MpuProcessorBase> MpuProcessorFactory::create(uint32_t tag) {
	switch (tag) {
	case MpuVideoProcessor::kAssetType:
		return std::make_shared<MpuVideoProcessor>();
	case MpuAudioProcessor::kAssetType:
		return std::make_shared<MpuAudioProcessor>();
	case MpuSubtitleProcessor::kAssetType:
		return std::make_shared<MpuSubtitleProcessor>();
	case MpuApplicationProcessor::kAssetType:
		return std::make_shared<MpuApplicationProcessor>();
	}

	return {};
}

}